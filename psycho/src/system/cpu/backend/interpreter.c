// SPDX-License-Identifier: MIT
//
// Copyright 2026 Michael Rodriguez
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <assert.h>
#include <string.h>

#include "common/util.h"
#include "system/sched.h"

#include "debug/bios_call_tracer.h"
#include "debug/disasm.h"
#include "debug/log.h"

#include "system/sbus/sbus.h"

#include "system/cpu/cpu.h"
#include "system/cpu/defs.h"
#include "exe_loader.h"
#include "interpreter.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_CPU);

#define LD_SLOT(m) (((struct psycho_ctx *)0)->cpu.base.m)

static_assert(sizeof(LD_SLOT(ld_pending)) == 8, "ld slot must be exactly 8 bytes");
static_assert(sizeof(LD_SLOT(ld_pending.dst)) == 4, "ld slot dst must be 32 bits");
static_assert(sizeof(LD_SLOT(ld_pending.val)) == 4, "ld slot val must be 32 bits");
static_assert(offsetof(__typeof__(LD_SLOT(ld_pending)), dst) == 0, "ld slot dst must come first");
static_assert(offsetof(__typeof__(LD_SLOT(ld_pending)), val) == 4, "ld slot val must come second");

#undef LD_SLOT

#define gte_clamp(ctx, val, min, max, flag)      \
	({                                       \
		if ((val) < (min)) {             \
			(val) = (min);           \
			flag_set((ctx), (flag)); \
		} else if ((val) > (max)) {      \
			(val) = (max);           \
			flag_set((ctx), (flag)); \
		}                                \
                                                 \
		(val);                           \
	})

PSYCHO_NONNULL static void reset(struct psycho_ctx *ctx)
{
	ctx->cpu.interpreter.slow = false;
	LOG_INFO(ctx, "reset");
}

PSYCHO_NONNULL static void stop(struct psycho_ctx *ctx)
{
	ctx->running		  = false;
	ctx->cpu.interpreter.slow = true;
}

PSYCHO_NODISCARD PSYCHO_NONNULL static uint32_t load_word(struct psycho_ctx *ctx, uint32_t vaddr, bool *valid)
{
	psycho_sched_ts_advance(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return psycho_bus_load_word(ctx, vaddr, valid);
}

PSYCHO_NODISCARD PSYCHO_NONNULL static uint16_t load_halfword(struct psycho_ctx *ctx, uint32_t vaddr, bool *valid)
{
	psycho_sched_ts_advance(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return psycho_bus_load_halfword(ctx, vaddr, valid);
}

PSYCHO_NODISCARD PSYCHO_NONNULL static uint8_t load_byte(struct psycho_ctx *ctx, uint32_t vaddr, bool *valid)
{
	psycho_sched_ts_advance(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return psycho_bus_load_byte(ctx, vaddr, valid);
}

PSYCHO_NONNULL static bool store_word(struct psycho_ctx *ctx, uint32_t vaddr, uint32_t data)
{
	psycho_sched_ts_advance(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return psycho_bus_store_word(ctx, vaddr, data);
}

PSYCHO_NONNULL static bool store_halfword(struct psycho_ctx *ctx, uint32_t vaddr, uint16_t data)
{
	psycho_sched_ts_advance(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return psycho_bus_store_halfword(ctx, vaddr, data);
}

PSYCHO_NONNULL static bool store_byte(struct psycho_ctx *ctx, uint32_t vaddr, uint8_t data)
{
	psycho_sched_ts_advance(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return psycho_bus_store_byte(ctx, vaddr, data);
}

PSYCHO_NONNULL static void gpr_set(struct psycho_ctx *ctx, size_t reg, uint32_t val)
{
	// If the instruction following a load writes to the same destination register, the load’s delay slot is
	// canceled.
	if (unlikely(ctx->cpu.base.ld_next.dst == reg))
		memset(&ctx->cpu.base.ld_next, 0, sizeof(ctx->cpu.base.ld_next));

	// Don't bother putting a check for a write to gpr[0] here; it's already bad enough that we have a branch.
	// gpr[0] is unconditionally set to 0 at the end of every step.
	ctx->cpu.base.gpr[reg] = val;
}

PSYCHO_NONNULL static void branch(struct psycho_ctx *ctx, uint32_t addr)
{
	ctx->cpu.base.next_in_bd = true;
	ctx->cpu.base.next_pc	 = addr;
}

PSYCHO_NONNULL static void branch_if(struct psycho_ctx *ctx, uint32_t instr, bool cond)
{
	ctx->cpu.base.next_in_bd = true;

	uint32_t base	= ctx->cpu.base.delay_pc - sizeof(uint32_t);
	uint32_t target = branch_addr(base, instr);
	uint32_t mask	= -cond;

	ctx->cpu.base.next_pc = (target & mask) | (ctx->cpu.base.next_pc & ~mask);
}

PSYCHO_NONNULL static void do_exception(struct psycho_ctx *ctx, enum cpu_exception exc, uint32_t vec)
{
#define CAUSE (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_CAUSE])
#define EPC   (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_EPC])
#define SR    (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	// So, on an exception, the CPU:

	// 1) sets up EPC to point to the restart location.
	EPC = ctx->cpu.base.pc;

	// 2) the pre-existing user-mode and interrupt-enable flags in SR are saved by pushing the 3-entry stack inside
	//    SR, and changing to kernel mode with interrupts disabled.
	SR = (SR & ~0x3F) | ((SR << 2) & 0x3F);

	// 3) Cause is setup so that software can see the reason for the exception.
	//
	//    Clear everything except the IP bits.
	CAUSE = (CAUSE & 0x0000FF00) | (exc << 2);

	// 4) transfers control to the exception entry point.
	psycho_cpu_pc_set(ctx, vec);

#undef CAUSE
#undef EPC
#undef SR
}

PSYCHO_NONNULL static void raise_exception(struct psycho_ctx *ctx, enum cpu_exception exc)
{
	do_exception(ctx, exc, VECTOR_EXCEPTION);
}

PSYCHO_NONNULL static void raise_debug_exception(struct psycho_ctx *ctx)
{
	do_exception(ctx, EXCEPTION_BP, VECTOR_DEBUG);
}

PSYCHO_NONNULL static void dbg_bp_write(struct psycho_ctx *ctx, uint32_t vaddr)
{
#define BDA  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_BDA])
#define BDAM (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_BDAM])
#define DCIC (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_DCIC])

	if (unlikely((DCIC & DCIC_BP_WRITE_EN_MASK) && !((vaddr ^ BDA) & BDAM))) {
		DCIC |= (DCIC_DB | DCIC_W);
		raise_debug_exception(ctx);
	}

#undef BDA
#undef BDAM
#undef DCIC
}

PSYCHO_NONNULL static void do_div(struct psycho_ctx *ctx, int32_t dividend, int32_t divisor)
{
#define LO (ctx->cpu.base.lo)
#define HI (ctx->cpu.base.hi)

	if (unlikely(!divisor)) {
		// That is, if the dividend is negative, the quotient is 1 (0x00000001), and if the dividend is positive
		// or zero, the quotient is -1 (0xFFFFFFFF).
		LO = (dividend < 0) ? 1 : UINT32_MAX;

		// In both cases the remainder equals the dividend.
		HI = dividend;
	} else if (unlikely((dividend == INT32_MIN) && (divisor == -1))) {
		LO = dividend;
		HI = 0;
	} else {
		LO = dividend / divisor;
		HI = dividend % divisor;
	}

#undef LO
#undef HI
}

PSYCHO_NONNULL static void do_divu(struct psycho_ctx *ctx, uint32_t dividend, uint32_t divisor)
{
#define LO (ctx->cpu.base.lo)
#define HI (ctx->cpu.base.hi)

	if (unlikely(!divisor)) {
		// In the case of unsigned division, the dividend can't be negative and thus the quotient is always -1
		// (0xFFFFFFFF) and the remainder equals the dividend.
		LO = UINT32_MAX;
		HI = dividend;
	} else {
		LO = dividend / divisor;
		HI = dividend % divisor;
	}

#undef LO
#undef HI
}

PSYCHO_NONNULL static void do_add(struct psycho_ctx *ctx, size_t dst, uint32_t a0, uint32_t a1)
{
	int sum;

	if (unlikely(__builtin_sadd_overflow(a0, a1, &sum)))
		raise_exception(ctx, EXCEPTION_OV);
	else
		gpr_set(ctx, dst, sum);
}

PSYCHO_NONNULL static void do_sub(struct psycho_ctx *ctx, size_t dst, uint32_t minuend, uint32_t subtrahend)
{
	int diff;

	if (unlikely(__builtin_ssub_overflow(minuend, subtrahend, &diff)))
		raise_exception(ctx, EXCEPTION_OV);
	else
		gpr_set(ctx, dst, diff);
}

PSYCHO_NONNULL static void update_flag(struct psycho_ctx *ctx)
{
#define FLAG (ctx->cpu.base.cop2.ccr.flag)

	if (FLAG & FLAG_ERR_MASK)
		FLAG |= FLAG_ERR;
	else
		FLAG &= ~FLAG_ERR;

#undef FLAG
}

PSYCHO_NONNULL static void flag_set(struct psycho_ctx *ctx, uint32_t flags)
{
#define FLAG (ctx->cpu.base.cop2.ccr.flag)

	FLAG |= flags;

#undef FLAG
}

PSYCHO_NONNULL static int64_t mac123_chk(struct psycho_ctx *ctx, int64_t sum, int64_t addend, uint32_t neg,
					 uint32_t pos)
{
	sum += addend;

	if (sum > MAC123_MAX)
		flag_set(ctx, pos);
	else if (sum < MAC123_MIN)
		flag_set(ctx, neg);

	return (int64_t)((uint64_t)sum << 20) >> 20;
}

PSYCHO_NONNULL static int64_t mac0_add(struct psycho_ctx *ctx, int64_t res)
{
	if (res > INT32_MAX)
		flag_set(ctx, MAC0_OVERFLOW_POS);
	else if (res < INT32_MIN)
		flag_set(ctx, MAC0_OVERFLOW_NEG);

	return res;
}

PSYCHO_NONNULL static int64_t mac123_add(struct psycho_ctx *ctx, size_t mac, int64_t sum, int64_t addend)
{
	switch (mac) {
	case 1:
		return mac123_chk(ctx, sum, addend, MAC1_OVERFLOW_NEG, MAC1_OVERFLOW_POS);

	case 2:
		return mac123_chk(ctx, sum, addend, MAC2_OVERFLOW_NEG, MAC2_OVERFLOW_POS);

	case 3:
		return mac123_chk(ctx, sum, addend, MAC3_OVERFLOW_NEG, MAC3_OVERFLOW_POS);

	default:
		unreachable();
	}
}

PSYCHO_NONNULL PSYCHO_NODISCARD static int16_t ir0_sat(struct psycho_ctx *ctx, int64_t val)
{
	return gte_clamp(ctx, val, IR0_MIN, IR0_MAX, IR0_SATURATED);
}

PSYCHO_NONNULL static int16_t ir123_sat(struct psycho_ctx *ctx, unsigned int ir, int32_t val, bool lm)
{
	int16_t min = lm ? IR123_LM_MIN : IR123_MIN;

	switch (ir) {
	case 1:
		return gte_clamp(ctx, val, min, IR123_MAX, IR1_SATURATED);

	case 2:
		return gte_clamp(ctx, val, min, IR123_MAX, IR2_SATURATED);

	case 3:
		return gte_clamp(ctx, val, min, IR123_MAX, IR3_SATURATED);

	default:
		unreachable();
	}
}

PSYCHO_NONNULL static void sx_push(struct psycho_ctx *ctx, int64_t val)
{
#define SXY (ctx->cpu.base.cop2.cpr.sxy)

	for (size_t i = 0; i < ARRAY_SIZE(SXY) - 1; ++i)
		SXY[i].x = SXY[i + 1].x;

	SXY[2].x = gte_clamp(ctx, val, SXY_MIN, SXY_MAX, SX2_SATURATED);

#undef SXY
}

PSYCHO_NONNULL static void sy_push(struct psycho_ctx *ctx, int64_t val)
{
#define SXY (ctx->cpu.base.cop2.cpr.sxy)

	for (size_t i = 0; i < ARRAY_SIZE(SXY) - 1; ++i)
		SXY[i].y = SXY[i + 1].y;

	SXY[2].y = gte_clamp(ctx, val, SXY_MIN, SXY_MAX, SY2_SATURATED);

#undef SXY
}

PSYCHO_NONNULL static void sz_push(struct psycho_ctx *ctx, int64_t val)
{
#define SZ (ctx->cpu.base.cop2.cpr.sz)

	for (size_t i = 0; i < ARRAY_SIZE(SZ) - 1; ++i)
		SZ[i].val = SZ[i + 1].val;

	SZ[3].val = gte_clamp(ctx, val, SZ_OTZ_MIN, SZ_OTZ_MAX, SZ3_OTZ_SATURATED);

#undef SZ
}

PSYCHO_NONNULL static uint16_t otz_set(struct psycho_ctx *ctx, int64_t val)
{
	return gte_clamp(ctx, val, SZ_OTZ_MIN, SZ_OTZ_MAX, SZ3_OTZ_SATURATED);
}

PSYCHO_NONNULL static uint32_t rgb_set(struct psycho_ctx *ctx, unsigned int color, int32_t val)
{
	switch (color) {
	case 0:
		return gte_clamp(ctx, val, RGB_MIN, RGB_MAX, RGB_R_SATURATED);

	case 1:
		return gte_clamp(ctx, val, RGB_MIN, RGB_MAX, RGB_G_SATURATED);

	case 2:
		return gte_clamp(ctx, val, RGB_MIN, RGB_MAX, RGB_B_SATURATED);

	default:
		unreachable();
	}
}

PSYCHO_NONNULL PSYCHO_NODISCARD static int64_t gte_div(struct psycho_ctx *ctx)
{
	uint32_t h   = ctx->cpu.base.cop2.ccr.h;
	uint32_t sz3 = ctx->cpu.base.cop2.cpr.sz[3].val;

	if (unlikely(h >= (sz3 * 2))) {
		flag_set(ctx, DIV_OVERFLOW);
		return 0x1FFFF;
	}

	int z	   = __builtin_clz(sz3) - 16;
	uint32_t d = sz3 << z;
	uint32_t u = psycho_gte_unr_tbl[(d - 0x7FC0) >> 7] + 0x101;

	d = (0x2000080 - (d * u)) >> 8;
	d = (0x0000080 + (d * u)) >> 8;

	uint64_t n = (((uint64_t)(h << z) * d) + 0x8000) >> 16;
	return min(n, UINT64_C(0x1FFFF));
}

PSYCHO_NONNULL static void color_fifo_push(struct psycho_ctx *ctx)
{
#define MAC  (ctx->cpu.base.cop2.cpr.mac)
#define RGB  (ctx->cpu.base.cop2.cpr.rgb)
#define RGBC (ctx->cpu.base.cop2.cpr.rgbc)

	uint32_t r = rgb_set(ctx, 0, (uint32_t)(MAC[1] >> 4)) << 0;
	uint32_t g = rgb_set(ctx, 1, (uint32_t)(MAC[2] >> 4)) << 8;
	uint32_t b = rgb_set(ctx, 2, (uint32_t)(MAC[3] >> 4)) << 16;
	uint32_t c = (uint32_t)RGBC.code << 24;

	RGB[0]	   = RGB[1];
	RGB[1]	   = RGB[2];
	RGB[2].val = r | g | b | c;

#undef MAC
#undef RGB
#undef RGBC
}

PSYCHO_NONNULL static void rtp(struct psycho_ctx *ctx, struct psycho_gte_vector *vec, bool dq)
{
#define DQA (ctx->cpu.base.cop2.ccr.dqa)
#define DQB (ctx->cpu.base.cop2.ccr.dqb)
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define MAC (ctx->cpu.base.cop2.cpr.mac)
#define OFX (ctx->cpu.base.cop2.ccr.ofx)
#define OFY (ctx->cpu.base.cop2.ccr.ofy)
#define RT  (ctx->cpu.base.cop2.ccr.r)
#define TR  (ctx->cpu.base.cop2.ccr.tr)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	int64_t sum;

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		sum = 0;
		sum = mac123_add(ctx, i, sum, (uint64_t)TR[j] << SHIFT_FRAC);
		sum = mac123_add(ctx, i, sum, RT[j][0] * vec->x);
		sum = mac123_add(ctx, i, sum, RT[j][1] * vec->y);
		sum = mac123_add(ctx, i, sum, RT[j][2] * vec->z);

		// The last iteration of this loop will give us the 44-bit MAC3; we'll need it to handle the special IR3
		// case
		MAC[i] = sum >> sf;
	}

	sum >>= SHIFT_FRAC;

	for (size_t i = 1; i < ARRAY_SIZE(MAC) - 1; ++i)
		IR[i] = ir123_sat(ctx, i, MAC[i], lm);

	(void)ir123_sat(ctx, 3, sum, false);
	IR[3] = clamp(MAC[3], lm ? IR123_LM_MIN : IR123_MIN, IR123_MAX);

	sz_push(ctx, sum);

	int64_t quot = gte_div(ctx);

	sum = mac0_add(ctx, (quot * IR[1]) + OFX);
	sx_push(ctx, sum >> 16);

	sum = mac0_add(ctx, (quot * IR[2]) + OFY);
	sy_push(ctx, sum >> 16);

	if (dq) {
		sum   = mac0_add(ctx, (quot * DQA) + DQB);
		IR[0] = ir0_sat(ctx, sum >> SHIFT_FRAC);
	}
	MAC[0] = sum;

#undef DQA
#undef DQB
#undef IR
#undef MAC
#undef OFX
#undef OFY
#undef RT
#undef TR
}

PSYCHO_NONNULL static void intpl_llm_vec(struct psycho_ctx *ctx, struct psycho_gte_vector *vec)
{
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define LLM (ctx->cpu.base.cop2.ccr.llm)
#define MAC (ctx->cpu.base.cop2.cpr.mac)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		int64_t sum = 0;
		sum	    = mac123_add(ctx, i, sum, LLM[j][0] * vec->x);
		sum	    = mac123_add(ctx, i, sum, LLM[j][1] * vec->y);
		sum	    = mac123_add(ctx, i, sum, LLM[j][2] * vec->z);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef IR
#undef LLM
#undef MAC
}

PSYCHO_NONNULL static void intpl_bk_lcm(struct psycho_ctx *ctx)
{
#define BK  (ctx->cpu.base.cop2.ccr.bk)
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define LCM (ctx->cpu.base.cop2.ccr.lcm)
#define MAC (ctx->cpu.base.cop2.cpr.mac)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		int64_t sum = 0;
		sum	    = mac123_add(ctx, i, sum, (uint64_t)BK[j] << SHIFT_FRAC);
		sum	    = mac123_add(ctx, i, sum, LCM[j][0] * IR[1]);
		sum	    = mac123_add(ctx, i, sum, LCM[j][1] * IR[2]);
		sum	    = mac123_add(ctx, i, sum, LCM[j][2] * IR[3]);

		MAC[i] = sum >> sf;
	}

	for (size_t i = 1; i < ARRAY_SIZE(MAC); ++i)
		IR[i] = ir123_sat(ctx, i, MAC[i], lm);

#undef BK
#undef IR
#undef LCM
#undef MAC
}

PSYCHO_NONNULL static void intpl_rgb(struct psycho_ctx *ctx)
{
#define IR   (ctx->cpu.base.cop2.cpr.ir)
#define MAC  (ctx->cpu.base.cop2.cpr.mac)
#define RGBC (ctx->cpu.base.cop2.cpr.rgbc.array)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		MAC[i] = (((uint64_t)(RGBC[j] * IR[i])) << 4) >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef IR
#undef MAC
#undef RGBC
}

PSYCHO_NONNULL static void intpl_fc(struct psycho_ctx *ctx, int64_t *sums)
{
#define FC  (ctx->cpu.base.cop2.ccr.fc)
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define MAC (ctx->cpu.base.cop2.cpr.mac)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		int64_t sum = 0;

		sum = mac123_add(ctx, i, 0, ((uint64_t)FC[j] << SHIFT_FRAC) - sums[j]);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], false);

		sum = 0;
		sum = mac123_add(ctx, i, sum, sums[j]);
		sum = mac123_add(ctx, i, sum, IR[i] * IR[0]);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef FC
#undef IR
#undef MAC
}

PSYCHO_NONNULL static void dpc(struct psycho_ctx *ctx, uint8_t *rgb)
{
	int64_t sums[3];

	for (size_t i = 0; i < ARRAY_SIZE(sums); ++i)
		sums[i] = (uint32_t)rgb[i] << 16;

	intpl_fc(ctx, sums);
	color_fifo_push(ctx);
}

PSYCHO_NONNULL static void nc(struct psycho_ctx *ctx, struct psycho_gte_vector *vec)
{
	intpl_llm_vec(ctx, vec);
	intpl_bk_lcm(ctx);
	color_fifo_push(ctx);
}

PSYCHO_NONNULL static void ncc(struct psycho_ctx *ctx, struct psycho_gte_vector *vec)
{
	intpl_llm_vec(ctx, vec);
	intpl_bk_lcm(ctx);
	intpl_rgb(ctx);
	color_fifo_push(ctx);
}

PSYCHO_NONNULL static void ncd(struct psycho_ctx *ctx, struct psycho_gte_vector *vec)
{
#define IR   (ctx->cpu.base.cop2.cpr.ir)
#define RGBC (ctx->cpu.base.cop2.cpr.rgbc.array)

	intpl_llm_vec(ctx, vec);
	intpl_bk_lcm(ctx);

	int64_t sums[3];

	for (size_t i = 0, j = 1; i < ARRAY_SIZE(sums); ++i, ++j)
		sums[i] = ((uint64_t)(RGBC[i] * IR[j])) << 4;

	intpl_fc(ctx, sums);
	color_fifo_push(ctx);

#undef IR
#undef RGBC
}

PSYCHO_NONNULL static void avsz(struct psycho_ctx *ctx, int16_t scale, size_t sz_off)
{
#define FLAG (ctx->cpu.base.cop2.ccr.flag)
#define MAC0 (ctx->cpu.base.cop2.cpr.mac[0])
#define OTZ  (ctx->cpu.base.cop2.cpr.otz)
#define SZ   (ctx->cpu.base.cop2.cpr.sz)

	FLAG = 0;

	int64_t sum = 0;

	while (sz_off < ARRAY_SIZE(SZ))
		sum += SZ[sz_off++].val;

	sum *= scale;

	MAC0 = mac0_add(ctx, sum);
	OTZ  = otz_set(ctx, sum >> SHIFT_FRAC);

	update_flag(ctx);

#undef FLAG
#undef MAC0
#undef OTZ
#undef SZ
}

PSYCHO_NONNULL static void do_cop2_mfc(struct psycho_ctx *ctx, size_t rt, size_t rd)
{
#define IR   (ctx->cpu.base.cop2.cpr.ir)
#define SXY2 (ctx->cpu.base.cop2.cpr.sxy[2].val)

	switch (rd) {
	case PSYCHO_CPU_COP2_CPR_SXYP:
		gpr_set(ctx, rt, SXY2);
		break;

	case PSYCHO_CPU_COP2_CPR_IRGB:
	case PSYCHO_CPU_COP2_CPR_ORGB: {
		uint32_t r = clamp(IR[1] >> 7, 0x00, 0x1F) << 0;
		uint32_t g = clamp(IR[2] >> 7, 0x00, 0x1F) << 5;
		uint32_t b = clamp(IR[3] >> 7, 0x00, 0x1F) << 10;

		gpr_set(ctx, rt, b | g | r);
		break;
	}

	default:
		gpr_set(ctx, rt, ctx->cpu.base.cop2.cpr.array[rd]);
		break;
	}

#undef IR
#undef SXY2
}

PSYCHO_NONNULL static void do_cop2_cfc(struct psycho_ctx *ctx, size_t rt, size_t rd)
{
#define H (ctx->cpu.base.cop2.ccr.h)

	if (rd == PSYCHO_CPU_COP2_CCR_H)
		gpr_set(ctx, rt, sign_extend_16_32(H));
	else
		gpr_set(ctx, rt, ctx->cpu.base.cop2.ccr.array[rd]);
#undef H
}

PSYCHO_NONNULL static void do_cop2_mtc(struct psycho_ctx *ctx, size_t rd, size_t rt)
{
#define gpr  (ctx->cpu.base.gpr)
#define IR   (ctx->cpu.base.cop2.cpr.ir)
#define LZCR (ctx->cpu.base.cop2.cpr.lzcr)
#define LZCS (ctx->cpu.base.cop2.cpr.lzcs)
#define SXY  (ctx->cpu.base.cop2.cpr.sxy)

	switch (rd) {
	case PSYCHO_CPU_COP2_CPR_OTZ:
	case PSYCHO_CPU_COP2_CPR_SZ0:
	case PSYCHO_CPU_COP2_CPR_SZ1:
	case PSYCHO_CPU_COP2_CPR_SZ2:
	case PSYCHO_CPU_COP2_CPR_SZ3:
		ctx->cpu.base.cop2.cpr.array[rd] = zero_extend_16_32(gpr[rt]);
		break;

	case PSYCHO_CPU_COP2_CPR_VZ0:
	case PSYCHO_CPU_COP2_CPR_VZ1:
	case PSYCHO_CPU_COP2_CPR_VZ2:
	case PSYCHO_CPU_COP2_CPR_IR0:
	case PSYCHO_CPU_COP2_CPR_IR1:
	case PSYCHO_CPU_COP2_CPR_IR2:
	case PSYCHO_CPU_COP2_CPR_IR3:
		ctx->cpu.base.cop2.cpr.array[rd] = sign_extend_16_32(gpr[rt]);
		break;

	case PSYCHO_CPU_COP2_CPR_IRGB:
		IR[1] = ((gpr[rt] >> 0) & 0x1F) << 7;
		IR[2] = ((gpr[rt] >> 5) & 0x1F) << 7;
		IR[3] = ((gpr[rt] >> 10) & 0x1F) << 7;

		break;

	case PSYCHO_CPU_COP2_CPR_SXYP:
		SXY[0]	   = SXY[1];
		SXY[1]	   = SXY[2];
		SXY[2].val = gpr[rt];

		break;

	case PSYCHO_CPU_COP2_CPR_LZCS: {
		LZCS = gpr[rt];

		uint32_t res = 32;

		if (LZCS > 0)
			// Reading LZCR returns the leading 0 count of LZCS if LZCS is positive...
			res = __builtin_clz(LZCS);
		else if (LZCS < 0)
			// ...and the leading 1 count of LZCS if LZCS is negative.
			res = (LZCS == -1) ? 32 : __builtin_clz(~LZCS);

		// The results are in range 1..32.
		LZCR = res;
		break;
	}

	case PSYCHO_CPU_COP2_CPR_LZCR:
		// Read-only register
		break;

	default:
		ctx->cpu.base.cop2.cpr.array[rd] = gpr[rt];
		break;
	}

#undef gpr
#undef IR
#undef LZCR
#undef LZCS
#undef SXY
}

PSYCHO_NONNULL static void do_cop2_ctc(struct psycho_ctx *ctx, size_t rd, size_t rt)
{
#define gpr  (ctx->cpu.base.gpr)
#define FLAG (ctx->cpu.base.cop2.ccr.flag)

	switch (rd) {
	case PSYCHO_CPU_COP2_CCR_R33:
	case PSYCHO_CPU_COP2_CCR_L33:
	case PSYCHO_CPU_COP2_CCR_DQA:
	case PSYCHO_CPU_COP2_CCR_LB3:
	case PSYCHO_CPU_COP2_CCR_ZSF3:
	case PSYCHO_CPU_COP2_CCR_ZSF4:
		ctx->cpu.base.cop2.ccr.array[rd] = sign_extend_16_32(gpr[rt]);
		break;

	case PSYCHO_CPU_COP2_CCR_FLAG:
		FLAG = gpr[rt] & FLAG_MASK;
		update_flag(ctx);

		break;

	default:
		ctx->cpu.base.cop2.ccr.array[rd] = gpr[rt];
		break;
	}

#undef gpr
#undef FLAG
}

static void mvmva(struct psycho_ctx *ctx, int16_t (*Mx)[3], int16_t *Vx, int32_t *Tx)
{
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define MAC (ctx->cpu.base.cop2.cpr.mac)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		int64_t sum = 0;
		sum	    = mac123_add(ctx, i, sum, (uint64_t)Tx[j] << SHIFT_FRAC);
		sum	    = mac123_add(ctx, i, sum, Mx[j][0] * Vx[0]);
		sum	    = mac123_add(ctx, i, sum, Mx[j][1] * Vx[1]);
		sum	    = mac123_add(ctx, i, sum, Mx[j][2] * Vx[2]);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef IR
#undef MAC
}

static void mvmva_bugged(struct psycho_ctx *ctx, int16_t (*Mx)[3], int16_t *Vx)
{
#define FC  (ctx->cpu.base.cop2.ccr.fc)
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define MAC (ctx->cpu.base.cop2.cpr.mac)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		int64_t sum = 0;
		sum	    = mac123_add(ctx, i, sum, (uint64_t)FC[j] << SHIFT_FRAC);
		sum	    = mac123_add(ctx, i, sum, Mx[j][0] * Vx[0]);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], false);

		sum = 0;
		sum = mac123_add(ctx, i, sum, Mx[j][1] * Vx[1]);
		sum = mac123_add(ctx, i, sum, Mx[j][2] * Vx[2]);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef FC
#undef IR
#undef MAC
}

PSYCHO_NONNULL static void delay_slot_process(struct psycho_ctx *ctx)
{
	if (likely(!ctx->cpu.base.ld_next.dst && !ctx->cpu.base.ld_pending.dst))
		return;

	ctx->cpu.base.gpr[ctx->cpu.base.ld_next.dst] = ctx->cpu.base.ld_next.val;

	if (ctx->cpu.base.ld_next.dst)
		LOG_TRACE(ctx, "load delay eviction: %u <- 0x%08X", ctx->cpu.base.ld_next.dst,
			  ctx->cpu.base.ld_next.val);

	ctx->cpu.base.ld_next = ctx->cpu.base.ld_pending;
	memset(&ctx->cpu.base.ld_pending, 0, sizeof(ctx->cpu.base.ld_pending));
}

PSYCHO_NONNULL static void gpr_set_delayed(struct psycho_ctx *ctx, size_t dst, uint32_t val)
{
	if (unlikely(!dst)) {
		LOG_DEBUG(ctx, "Load delay rejected - dest was $zero");
		return;
	}

	// One 8-byte store, so the 8-byte copy in dly_slot_process() can forward from it.
	uint64_t packed = (uint64_t)dst | ((uint64_t)val << 32);
	memcpy(&ctx->cpu.base.ld_pending, &packed, sizeof(packed));

	LOG_TRACE(ctx, "load delay pending: dst=%zu, val=0x%08X", dst, val);

	if (unlikely(ctx->cpu.base.ld_next.dst == dst))
		memset(&ctx->cpu.base.ld_next, 0, sizeof(ctx->cpu.base.ld_next));
}

PSYCHO_NONNULL static void do_jalr(struct psycho_ctx *ctx, size_t rs, size_t rd)
{
#define gpr   (ctx->cpu.base.gpr)
#define instr (ctx->cpu.base.instr)
#define pc    (ctx->cpu.base.pc)

	uint32_t addr = gpr[rs];

	gpr_set(ctx, rd, pc + (sizeof(instr) * 2));
	branch(ctx, addr);

	if (unlikely(ctx->cpu.base.next_pc & 3))
		ctx->cpu.interpreter.slow = true;

#undef gpr
#undef instr
#undef pc
}

PSYCHO_NONNULL static void do_mult(struct psycho_ctx *ctx, size_t rs, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define hi  (ctx->cpu.base.hi)
#define lo  (ctx->cpu.base.lo)

	uint64_t prod = sign_extend_32_64(gpr[rs]) * sign_extend_32_64(gpr[rt]);

	lo = prod & UINT32_MAX;
	hi = prod >> 32;

#undef gpr
#undef hi
#undef lo
}

PSYCHO_NONNULL static void do_multu(struct psycho_ctx *ctx, size_t rs, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define hi  (ctx->cpu.base.hi)
#define lo  (ctx->cpu.base.lo)

	uint64_t prod = zero_extend_32_64(gpr[rs]) * zero_extend_32_64(gpr[rt]);

	lo = prod & UINT32_MAX;
	hi = prod >> 32;

#undef gpr
#undef hi
#undef lo
}

PSYCHO_NONNULL static void regimm(struct psycho_ctx *ctx, size_t rs, size_t rt)
{
#define gpr   (ctx->cpu.base.gpr)
#define pc    (ctx->cpu.base.pc)
#define instr (ctx->cpu.base.instr)

	bool link   = (rt & 0x1E) == 0x10;
	bool branch = (int32_t)(gpr[rs] ^ (rt << 31)) < 0;

	if (link)
		gpr_set(ctx, PSYCHO_CPU_GPR_RA, pc + (sizeof(instr) * 2));

	branch_if(ctx, instr, branch);

#undef gpr
#undef pc
#undef instr
}

PSYCHO_NONNULL static void do_addi(struct psycho_ctx *ctx, size_t rs, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)

	int sum;

	uint32_t a0 = gpr[rs];
	uint32_t a1 = sign_extend_16_32(field_get16(INSTR_IMMEDIATE, ctx->cpu.base.instr));

	if (unlikely(__builtin_sadd_overflow(a0, a1, &sum)))
		raise_exception(ctx, EXCEPTION_OV);
	else
		gpr_set(ctx, rt, sum);

#undef gpr
}

PSYCHO_NONNULL static void do_op(struct psycho_ctx *ctx)
{
#define D1  (ctx->cpu.base.cop2.ccr.r[0][0])
#define D2  (ctx->cpu.base.cop2.ccr.r[1][1])
#define D3  (ctx->cpu.base.cop2.ccr.r[2][2])
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define MAC (ctx->cpu.base.cop2.cpr.mac)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	int64_t res[3] = {
		// clang-format off

		[0] = (IR[3] * D2) - (IR[2] * D3),
		[1] = (IR[1] * D3) - (IR[3] * D1),
		[2] = (IR[2] * D1) - (IR[1] * D2)

		// clang-format on
	};

	for (size_t i = 0, reg = 1; i < ARRAY_SIZE(res); ++i, ++reg) {
		int64_t sum = 0;
		sum	    = mac123_add(ctx, reg, sum, res[i]);

		MAC[reg] = sum >> sf;
		IR[reg]	 = ir123_sat(ctx, reg, MAC[reg], lm);
	}

#undef D1
#undef D2
#undef D3
#undef IR
#undef MAC
}

PSYCHO_NONNULL static void do_intpl(struct psycho_ctx *ctx)
{
#define IR (ctx->cpu.base.cop2.cpr.ir)

	int64_t sums[3];

	for (size_t i = 0, j = 1; i < ARRAY_SIZE(sums); ++i, ++j)
		sums[i] = (uint64_t)IR[j] << SHIFT_FRAC;

	intpl_fc(ctx, sums);
	color_fifo_push(ctx);

#undef IR
}

PSYCHO_NONNULL static void do_mvmva(struct psycho_ctx *ctx)
{
#define BK   (ctx->cpu.base.cop2.ccr.bk)
#define FC   (ctx->cpu.base.cop2.ccr.fc)
#define IR   (ctx->cpu.base.cop2.cpr.ir)
#define LCM  (ctx->cpu.base.cop2.ccr.lcm)
#define LLM  (ctx->cpu.base.cop2.ccr.llm)
#define RGBC (ctx->cpu.base.cop2.cpr.rgbc.array)
#define RT   (ctx->cpu.base.cop2.ccr.r)
#define TR   (ctx->cpu.base.cop2.ccr.tr)
#define V    (ctx->cpu.base.cop2.cpr.vector)

	int16_t (*mx_lut[3])[3] = {
		// clang-format off

		[0] = RT,
		[1] = LLM,
		[2] = LCM

		// clang-format on
	};

	int32_t *tx_lut[4] = {
		// clang-format off

		[0] = TR,
		[1] = BK,
		[2] = FC,
		[3] = (int32_t[3]){ 0 }

		// clang-format on
	};

	unsigned int mx_sel = field_get32(INSTR_MVMVA_MX, ctx->cpu.base.instr);

	int16_t (*mx)[3];
	int16_t mx_bugged[3][3];

	if (likely(mx_sel <= 2))
		mx = mx_lut[mx_sel];
	else {
		mx_bugged[0][0] = (uint32_t)-RGBC[0] << 4;
		mx_bugged[0][1] = +RGBC[0] << 4;
		mx_bugged[0][2] = IR[0];
		mx_bugged[1][0] = RT[0][2];
		mx_bugged[1][1] = RT[0][2];
		mx_bugged[1][2] = RT[0][2];
		mx_bugged[2][0] = RT[1][1];
		mx_bugged[2][1] = RT[1][1];
		mx_bugged[2][2] = RT[1][1];

		mx = mx_bugged;
	}

	unsigned int tx_sel = field_get32(INSTR_MVMVA_TX, ctx->cpu.base.instr);
	int32_t *tx	    = tx_lut[tx_sel];

	unsigned int vx_sel = field_get32(INSTR_MVMVA_VX, ctx->cpu.base.instr);

	int16_t vx_tmp[3];
	int16_t *vx;

	if (vx_sel <= 2)
		vx = V[vx_sel].array;
	else {
		vx_tmp[0] = IR[1];
		vx_tmp[1] = IR[2];
		vx_tmp[2] = IR[3];
		vx	  = vx_tmp;
	}

	if (likely(tx_sel != 2))
		mvmva(ctx, mx, vx, tx);
	else
		// Don't need to worry about passing Tx - only the FC vector is buggy
		mvmva_bugged(ctx, mx, vx);

#undef BK
#undef FC
#undef IR
#undef LCM
#undef LLM
#undef RGBC
#undef RT
#undef TR
#undef V
}

PSYCHO_NONNULL static void do_cdp(struct psycho_ctx *ctx)
{
#define IR   (ctx->cpu.base.cop2.cpr.ir)
#define RGBC (ctx->cpu.base.cop2.cpr.rgbc.array)

	intpl_bk_lcm(ctx);

	int64_t sums[3];

	for (size_t i = 0, j = 1; i < ARRAY_SIZE(sums); ++i, ++j)
		sums[i] = ((uint64_t)(RGBC[i] * IR[j])) << 4;

	intpl_fc(ctx, sums);
	color_fifo_push(ctx);

#undef IR
#undef RGBC
}

PSYCHO_NONNULL static void do_sqr(struct psycho_ctx *ctx)
{
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define MAC (ctx->cpu.base.cop2.cpr.mac)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	for (size_t i = 1; i < ARRAY_SIZE(IR); ++i) {
		MAC[i] = ((int64_t)IR[i] * (int64_t)IR[i]) >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef IR
#undef MAC
}

PSYCHO_NONNULL static void do_dpcl(struct psycho_ctx *ctx)
{
#define IR   (ctx->cpu.base.cop2.cpr.ir)
#define RGBC (ctx->cpu.base.cop2.cpr.rgbc.array)

	int64_t sums[3];

	for (size_t i = 0, j = 1; i < ARRAY_SIZE(sums); ++i, ++j)
		sums[i] = ((uint64_t)(RGBC[i] * IR[j])) << 4;

	intpl_fc(ctx, sums);
	color_fifo_push(ctx);

#undef IR
#undef RGBC
}

PSYCHO_NONNULL static void do_gpl(struct psycho_ctx *ctx)
{
#define IR  (ctx->cpu.base.cop2.cpr.ir)
#define MAC (ctx->cpu.base.cop2.cpr.mac)

	unsigned int sf = shift_frac(ctx->cpu.base.instr);
	bool lm		= ir123_lm(ctx->cpu.base.instr);

	for (size_t i = 1; i < ARRAY_SIZE(MAC); ++i) {
		int64_t sum = 0;
		sum	    = mac123_add(ctx, i, sum, (uint64_t)MAC[i] << sf);
		sum	    = mac123_add(ctx, i, sum, IR[i] * IR[0]);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

	color_fifo_push(ctx);
	update_flag(ctx);

#undef IR
#undef MAC
}

PSYCHO_NONNULL static void do_lh(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)

	uint32_t vaddr = gpr[base] + offset;

	if (unlikely(vaddr & 1)) {
		raise_exception(ctx, EXCEPTION_ADEL);
		return;
	}

	bool valid;
	uint32_t data = sign_extend_16_32(load_halfword(ctx, vaddr, &valid));

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}

	gpr_set_delayed(ctx, rt, data);

#undef gpr
}

PSYCHO_NONNULL static void do_lwl(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr	       = gpr[base] + offset;
	uint32_t aligned_vaddr = vaddr & ~3;

	bool valid;
	uint32_t word = load_word(ctx, aligned_vaddr, &valid);

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}

	unsigned int shift = (vaddr & 3) * 8;
	unsigned int mask  = 0x00FFFFFF >> shift;

	uint32_t val = (ctx->cpu.base.ld_next.dst == rt) ? ctx->cpu.base.ld_next.val : gpr[rt];

	val = (val & mask) | (word << (24 - shift));
	gpr_set_delayed(ctx, rt, val);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_lw(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr = gpr[base] + offset;

	if (unlikely(vaddr & 0x3)) {
		raise_exception(ctx, EXCEPTION_ADEL);
		return;
	}

	bool valid;
	uint32_t word = load_word(ctx, vaddr, &valid);

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}

	gpr_set_delayed(ctx, rt, word);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_lhu(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr = gpr[base] + offset;

	if (unlikely(vaddr & 1)) {
		raise_exception(ctx, EXCEPTION_ADEL);
		return;
	}

	bool valid;
	uint32_t data = zero_extend_16_32(load_halfword(ctx, vaddr, &valid));

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}

	gpr_set_delayed(ctx, rt, data);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_lwr(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr	       = gpr[base] + offset;
	uint32_t aligned_vaddr = vaddr & ~3;

	bool valid;
	uint32_t word = load_word(ctx, aligned_vaddr, &valid);

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}

	unsigned int shift = (vaddr & 3) * 8;
	unsigned int mask  = 0xFFFFFF00 << (24 - shift);

	uint32_t val = (ctx->cpu.base.ld_next.dst == rt) ? ctx->cpu.base.ld_next.val : gpr[rt];

	val = (val & mask) | (word >> shift);

	gpr_set_delayed(ctx, rt, val);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_sb(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr = gpr[base] + offset;

	if (unlikely(!store_byte(ctx, vaddr, gpr[rt] & UINT8_MAX)))
		raise_exception(ctx, EXCEPTION_DBE);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_sh(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr = gpr[base] + offset;

	if (unlikely(vaddr & 1)) {
		raise_exception(ctx, EXCEPTION_ADES);
		return;
	}

	if (unlikely(!store_halfword(ctx, vaddr, gpr[rt] & UINT16_MAX)))
		raise_exception(ctx, EXCEPTION_DBE);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_swl(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr	       = gpr[base] + offset;
	uint32_t aligned_vaddr = vaddr & ~3;

	unsigned int shift = (vaddr & 3) * 8;
	unsigned int mask  = 0xFFFFFF00 << shift;

	bool valid;
	uint32_t word = load_word(ctx, aligned_vaddr, &valid);

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}

	word = (word & mask) | (gpr[rt] >> (24 - shift));

	if (unlikely(!store_word(ctx, aligned_vaddr, word)))
		raise_exception(ctx, EXCEPTION_DBE);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_sw(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr = gpr[base] + offset;

	if (unlikely(vaddr & 3)) {
		raise_exception(ctx, EXCEPTION_ADES);
		return;
	}

	dbg_bp_write(ctx, vaddr);

	if (unlikely(!store_word(ctx, vaddr, gpr[rt])))
		raise_exception(ctx, EXCEPTION_DBE);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_swr(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)
#define SR  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

	if (SR & SR_ISC)
		return;

	uint32_t vaddr	       = gpr[base] + offset;
	uint32_t aligned_vaddr = vaddr & ~3;

	unsigned int shift = (vaddr & 3) * 8;
	unsigned int mask  = 0x00FFFFFF >> (24 - shift);

	bool valid;
	uint32_t word = load_word(ctx, aligned_vaddr, &valid);

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}

	word = (word & mask) | (gpr[rt] << shift);

	if (unlikely(!store_word(ctx, aligned_vaddr, word)))
		raise_exception(ctx, EXCEPTION_DBE);

#undef gpr
#undef SR
}

PSYCHO_NONNULL static void do_lb(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)

	bool valid;
	uint32_t data = sign_extend_8_32(load_byte(ctx, gpr[base] + offset, &valid));

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}
	gpr_set_delayed(ctx, rt, data);

#undef gpr
}

PSYCHO_NONNULL static void do_lbu(struct psycho_ctx *ctx, size_t base, size_t offset, size_t rt)
{
#define gpr (ctx->cpu.base.gpr)

	bool valid;
	uint32_t data = zero_extend_8_32(load_byte(ctx, gpr[base] + offset, &valid));

	if (unlikely(!valid)) {
		raise_exception(ctx, EXCEPTION_DBE);
		return;
	}
	gpr_set_delayed(ctx, rt, data);

#undef gpr
}

PSYCHO_NONNULL static void disasm_trace_emit(struct psycho_ctx *ctx)
{
	psycho_disasm_trace_end(ctx);
	LOG_TRACE(ctx, "[disasm] 0x%08X: %s", ctx->disasm.res.pc, ctx->disasm.res.buf);
}

PSYCHO_NONNULL static void run(struct psycho_ctx *ctx, bool single_step)
{
#define gpr		  (ctx->cpu.base.gpr)
#define pc		  (ctx->cpu.base.pc)
#define next_pc		  (ctx->cpu.base.next_pc)
#define hi		  (ctx->cpu.base.hi)
#define lo		  (ctx->cpu.base.lo)

#define op		  (field_get32(INSTR_OP, instr))
#define rt		  (field_get32(INSTR_RT, instr))
#define rs		  (field_get32(INSTR_RS, instr))
#define rd		  (field_get32(INSTR_RD, instr))
#define shamt		  (field_get32(INSTR_SHAMT, instr))
#define funct		  (field_get32(INSTR_FUNCT, instr))
#define m_base		  (rs)
#define zextimm		  (zero_extend_16_32(field_get16(INSTR_IMMEDIATE, instr)))
#define sextimm		  (sign_extend_16_32(field_get16(INSTR_IMMEDIATE, instr)))
#define offset		  (sextimm)

#define SR		  (ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR])

#define FLAG		  (ctx->cpu.base.cop2.ccr.flag)
#define MAC		  (ctx->cpu.base.cop2.cpr.mac)
#define V		  (ctx->cpu.base.cop2.cpr.vector)
#define SX0		  (ctx->cpu.base.cop2.cpr.sxy[0].x)
#define SX1		  (ctx->cpu.base.cop2.cpr.sxy[1].x)
#define SX2		  (ctx->cpu.base.cop2.cpr.sxy[2].x)
#define SY0		  (ctx->cpu.base.cop2.cpr.sxy[0].y)
#define SY1		  (ctx->cpu.base.cop2.cpr.sxy[1].y)
#define SY2		  (ctx->cpu.base.cop2.cpr.sxy[2].y)
#define RGB0		  (ctx->cpu.base.cop2.cpr.rgb[0].array)
#define RGBC		  (ctx->cpu.base.cop2.cpr.rgbc.array)
#define ZSF3		  (ctx->cpu.base.cop2.ccr.zsf3)
#define ZSF4		  (ctx->cpu.base.cop2.ccr.zsf4)

#define OP(x)		  [64 + (x)]
#define OPR(a, b)	  [64 + (a)... 64 + (b)]

#define BIOS_TRACE_ON()	  (ctx->cfg.bios_call_tracer.enabled)
#define DISASM_TRACE_ON() (ctx->cfg.disasm.tracing)

#define FETCH()                                                                  \
	do {                                                                     \
		bool valid;                                                      \
                                                                                 \
		ctx->cpu.base.in_bd	 = ctx->cpu.base.next_in_bd;             \
		ctx->cpu.base.next_in_bd = false;                                \
                                                                                 \
		pc    = ctx->cpu.base.delay_pc;                                  \
		instr = load_word(ctx, pc, &valid);                              \
                                                                                 \
		if (unlikely(!valid))                                            \
			raise_exception(ctx, EXCEPTION_IBE);                     \
                                                                                 \
		ctx->cpu.base.instr    = instr;                                  \
		ctx->cpu.base.delay_pc = next_pc;                                \
		next_pc		       = ctx->cpu.base.delay_pc + sizeof(instr); \
                                                                                 \
		delay_slot_process(ctx);                                         \
	} while (0)

#define DISPATCH()                                                                             \
	do {                                                                                   \
		gpr[PSYCHO_CPU_GPR_ZERO] = 0;                                                  \
                                                                                               \
		if (unlikely((psycho_sched_run_required(ctx)) || (ctx->cpu.interpreter.slow))) \
			goto slow;                                                             \
                                                                                               \
		FETCH();                                                                       \
		goto *dispatch_tbl[op ? (64 + op) : funct];                                    \
	} while (0)

	static void *const dispatch_tbl[128] = {
		// clang-format off

		[SLL]		= &&op_sll,
		[0x01]		= &&illegal,
		[SRL]		= &&op_srl,
		[SRA]		= &&op_sra,
		[SLLV]		= &&op_sllv,
		[0x05]		= &&illegal,
		[SRLV]		= &&op_srlv,
		[SRAV]		= &&op_srav,
		[JR]		= &&op_jr,
		[JALR]		= &&op_jalr,
		[0x0A ... 0x0B]	= &&illegal,
		[SYSCALL]	= &&op_syscall,
		[BREAK]		= &&op_break,
		[0x0E ... 0x0F]	= &&illegal,
		[MFHI]		= &&op_mfhi,
		[MTHI]		= &&op_mthi,
		[MFLO]		= &&op_mflo,
		[MTLO]		= &&op_mtlo,
		[0x14 ... 0x17]	= &&illegal,
		[MULT]		= &&op_mult,
		[MULTU]		= &&op_multu,
		[DIV]		= &&op_div,
		[DIVU]		= &&op_divu,
		[0x1C ... 0x1F]	= &&illegal,
		[ADD]		= &&op_add,
		[ADDU]		= &&op_addu,
		[SUB]		= &&op_sub,
		[SUBU]		= &&op_subu,
		[AND]		= &&op_and,
		[OR]		= &&op_or,
		[XOR]		= &&op_xor,
		[NOR]		= &&op_nor,
		[0x28 ... 0x29]	= &&illegal,
		[SLT]		= &&op_slt,
		[SLTU]		= &&op_sltu,
		[0x2C ... 0x3F]	= &&illegal,
		[64]		= &&illegal,
		OP(GRP_REGIMM)	= &&regimm,
		OP(J)		= &&op_j,
		OP(JAL)		= &&op_jal,
		OP(BEQ)		= &&op_beq,
		OP(BNE)		= &&op_bne,
		OP(BLEZ)	= &&op_blez,
		OP(BGTZ)	= &&op_bgtz,
		OP(ADDI)	= &&op_addi,
		OP(ADDIU)	= &&op_addiu,
		OP(SLTI)	= &&op_slti,
		OP(SLTIU)	= &&op_sltiu,
		OP(ANDI)	= &&op_andi,
		OP(ORI)		= &&op_ori,
		OP(XORI)	= &&op_xori,
		OP(LUI)		= &&op_lui,
		OP(GRP_COP0)	= &&cop0,
		OP(0x11)	= &&illegal,
		OP(GRP_COP2)	= &&cop2,
		OPR(0x13, 0x1F)	= &&illegal,
		OP(LB)		= &&op_lb,
		OP(LH)		= &&op_lh,
		OP(LWL)		= &&op_lwl,
		OP(LW)		= &&op_lw,
		OP(LBU)		= &&op_lbu,
		OP(LHU)		= &&op_lhu,
		OP(LWR)		= &&op_lwr,
		OP(0x27)	= &&illegal,
		OP(SB)		= &&op_sb,
		OP(SH)		= &&op_sh,
		OP(SWL)		= &&op_swl,
		OP(SW)		= &&op_sw,
		OP(SWR)		= &&op_swr,
		OPR(0x2F, 0x3F)	= &&illegal

		// clang-format on
	};

#undef OP
#undef OPR

	static void *const cop0_tbl[] = {
		// clang-format off

		[MFC]		= &&cop0_mfc,
		[0x01 ... 0x03]	= &&cop0_instr,
		[MTC]		= &&cop0_mtc,
		[0x05 ... CTC]	= &&cop0_instr,
		[0x07 ... 0x1F]	= &&cop0_instr

		// clang-format on
	};

	static void *const cop0_instr_tbl[] = {
		// clang-format off

		[0x00 ... 0x0F]	= &&illegal,
		[RFE]		= &&op_rfe,
		[0x11 ... 0x3F]	= &&illegal

		// clang-format on
	};

	static void *const cop2_tbl[] = {
		// clang-format off

		[MFC]		= &&cop2_mfc,
		[0x01]		= &&cop2_instr,
		[CFC]		= &&cop2_cfc,
		[0x03]		= &&cop2_instr,
		[MTC]		= &&cop2_mtc,
		[0x05]		= &&cop2_instr,
		[CTC]		= &&cop2_ctc,
		[0x07 ... 0x1F]	= &&cop2_instr

		// clang-format on
	};

	static void *const cop2_instr_tbl[] = {
		// clang-format off

		[0x00]		= &&illegal,
		[RTPS]		= &&op_rtps,
		[0x02 ... 0x05]	= &&illegal,
		[NCLIP]		= &&op_nclip,
		[0x07 ... 0x0B]	= &&illegal,
		[OP]		= &&op_op,
		[0x0D ... 0x0F]	= &&illegal,
		[DPCS]		= &&op_dpcs,
		[INTPL]		= &&op_intpl,
		[MVMVA]		= &&op_mvmva,
		[NCDS]		= &&op_ncds,
		[CDP]		= &&op_cdp,
		[0x15]		= &&illegal,
		[NCDT]		= &&op_ncdt,
		[0x17 ... 0x1A]	= &&illegal,
		[NCCS]		= &&op_nccs,
		[CC]		= &&op_cc,
		[0x1D]		= &&illegal,
		[NCS]		= &&op_ncs,
		[0x1F]		= &&illegal,
		[NCT]		= &&op_nct,
		[0x21 ... 0x27]	= &&illegal,
		[SQR]		= &&op_sqr,
		[DPCL]		= &&op_dpcl,
		[DPCT]		= &&op_dpct,
		[0x2B ... 0x2C]	= &&illegal,
		[AVSZ3]		= &&op_avsz3,
		[AVSZ4]		= &&op_avsz4,
		[0x2F]		= &&illegal,
		[RTPT]		= &&op_rtpt,
		[0x31 ... 0x3C]	= &&illegal,
		[GPF]		= &&op_gpf,
		[GPL]		= &&op_gpl,
		[NCCT]		= &&op_ncct

		// clang-format on
	};

	uint32_t instr	   = 0;
	bool stepped	   = false;
	bool trace_pending = false;

	goto loop;

slow:
	if (unlikely(trace_pending)) {
		disasm_trace_emit(ctx);
		trace_pending = false;
	}

	if (unlikely(BIOS_TRACE_ON() && entering_bios_call(pc)))
		psycho_bios_call_tracer_begin(ctx, gpr[PSYCHO_CPU_GPR_T1], bios_call_tracer_pc_to_offset(pc));

	if (unlikely(BIOS_TRACE_ON() && leaving_bios_call(ctx, gpr[PSYCHO_CPU_GPR_RA], instr)))
		psycho_bios_call_tracer_end(ctx, gpr[PSYCHO_CPU_GPR_V0]);

loop:
	while (unlikely(psycho_sched_run_required(ctx)))
		psycho_sched_run(ctx);

	if (unlikely((!ctx->running) || stepped))
		goto done;

	stepped = single_step;

	if (unlikely((ctx->exe.data) && (pc == KERNEL_INIT_PC)))
		psycho_ctx_exe_inject(ctx);

	if (unlikely(ctx->cpu.base.delay_pc & 3))
		raise_exception(ctx, EXCEPTION_ADEL);

	ctx->cpu.interpreter.slow = ctx->exe.data || (next_pc & 3) || BIOS_TRACE_ON() || DISASM_TRACE_ON() ||
				    single_step;

	if (unlikely(DISASM_TRACE_ON())) {
		psycho_disasm_trace_begin(ctx, ctx->cpu.base.delay_pc);
		trace_pending = true;
	}

	FETCH();
	goto *dispatch_tbl[op ? (64 + op) : funct];

op_sll:
	gpr_set(ctx, rd, gpr[rt] << shamt);
	DISPATCH();

op_srl:
	gpr_set(ctx, rd, gpr[rt] >> shamt);
	DISPATCH();

op_sra:
	gpr_set(ctx, rd, (int32_t)gpr[rt] >> shamt);
	DISPATCH();

op_sllv:
	gpr_set(ctx, rd, gpr[rt] << (gpr[rs] & 0x1F));
	DISPATCH();

op_srlv:
	gpr_set(ctx, rd, gpr[rt] >> (gpr[rs] & 0x1F));
	DISPATCH();

op_srav:
	gpr_set(ctx, rd, (int32_t)gpr[rt] >> (gpr[rs] & 0x1F));
	DISPATCH();

op_jr:
	branch(ctx, gpr[rs]);

	if (unlikely(next_pc & 3))
		ctx->cpu.interpreter.slow = true;

	DISPATCH();

op_jalr:
	do_jalr(ctx, rs, rd);
	DISPATCH();

op_syscall:
	raise_exception(ctx, EXCEPTION_SYSCALL);
	DISPATCH();

op_break:
	raise_exception(ctx, EXCEPTION_BP);
	DISPATCH();

op_mfhi:
	gpr_set(ctx, rd, hi);
	DISPATCH();

op_mthi:
	hi = gpr[rs];
	DISPATCH();

op_mflo:
	gpr_set(ctx, rd, lo);
	DISPATCH();

op_mtlo:
	lo = gpr[rs];
	DISPATCH();

op_mult:
	do_mult(ctx, rs, rt);
	DISPATCH();

op_multu:
	do_multu(ctx, rs, rt);
	DISPATCH();

op_div:
	do_div(ctx, gpr[rs], gpr[rt]);
	DISPATCH();

op_divu:
	do_divu(ctx, gpr[rs], gpr[rt]);
	DISPATCH();

op_add:
	do_add(ctx, rd, gpr[rs], gpr[rt]);
	DISPATCH();

op_addu:
	gpr_set(ctx, rd, gpr[rs] + gpr[rt]);
	DISPATCH();

op_sub:
	do_sub(ctx, rd, gpr[rs], gpr[rt]);
	DISPATCH();

op_subu:
	gpr_set(ctx, rd, gpr[rs] - gpr[rt]);
	DISPATCH();

op_and:
	gpr_set(ctx, rd, gpr[rs] & gpr[rt]);
	DISPATCH();

op_or:
	gpr_set(ctx, rd, gpr[rs] | gpr[rt]);
	DISPATCH();

op_xor:
	gpr_set(ctx, rd, gpr[rs] ^ gpr[rt]);
	DISPATCH();

op_nor:
	gpr_set(ctx, rd, ~(gpr[rs] | gpr[rt]));
	DISPATCH();

op_slt:
	gpr_set(ctx, rd, (int32_t)gpr[rs] < (int32_t)gpr[rt]);
	DISPATCH();

op_sltu:
	gpr_set(ctx, rd, gpr[rs] < gpr[rt]);
	DISPATCH();

regimm:
	regimm(ctx, rs, rt);
	DISPATCH();

op_j:
	branch(ctx, jmp_addr(pc, instr));
	DISPATCH();

op_jal:
	gpr_set(ctx, PSYCHO_CPU_GPR_RA, pc + (sizeof(instr) * 2));
	branch(ctx, jmp_addr(pc, instr));

	DISPATCH();

op_beq:
	branch_if(ctx, instr, gpr[rs] == gpr[rt]);
	DISPATCH();

op_bne:
	branch_if(ctx, instr, gpr[rs] != gpr[rt]);
	DISPATCH();

op_blez:
	branch_if(ctx, instr, (int32_t)gpr[rs] <= 0);
	DISPATCH();

op_bgtz:
	branch_if(ctx, instr, (int32_t)gpr[rs] > 0);
	DISPATCH();

op_addi:
	do_addi(ctx, rs, rt);
	DISPATCH();

op_addiu:
	gpr_set(ctx, rt, gpr[rs] + sextimm);
	DISPATCH();

op_slti:
	gpr_set(ctx, rt, (int32_t)gpr[rs] < (int32_t)sextimm);
	DISPATCH();

op_sltiu:
	gpr_set(ctx, rt, gpr[rs] < sextimm);
	DISPATCH();

op_andi:
	gpr_set(ctx, rt, zextimm & gpr[rs]);
	DISPATCH();

op_ori:
	gpr_set(ctx, rt, zextimm | gpr[rs]);
	DISPATCH();

op_xori:
	gpr_set(ctx, rt, zextimm ^ gpr[rs]);
	DISPATCH();

op_lui:
	gpr_set(ctx, rt, zextimm << 16);
	DISPATCH();

cop0:
	goto *cop0_tbl[rs];

cop0_mfc:
	gpr_set(ctx, rt, ctx->cpu.base.cop0_cpr[rd]);
	DISPATCH();

cop0_mtc:
	ctx->cpu.base.cop0_cpr[rd] = gpr[rt];
	DISPATCH();

cop0_instr:
	goto *cop0_instr_tbl[funct];

op_rfe:
	SR = (SR & ~0x0F) | ((SR >> 2) & 0x0F);
	DISPATCH();

cop2:
	goto *cop2_tbl[rs];

cop2_mfc:
	do_cop2_mfc(ctx, rt, rd);
	DISPATCH();

cop2_cfc:
	do_cop2_cfc(ctx, rt, rd);
	DISPATCH();

cop2_mtc:
	do_cop2_mtc(ctx, rd, rt);
	DISPATCH();

cop2_ctc:
	do_cop2_ctc(ctx, rd, rt);
	DISPATCH();

cop2_instr:
	goto *cop2_instr_tbl[funct];

op_rtps:
	FLAG = 0;

	rtp(ctx, &V[0], true);
	update_flag(ctx);

	DISPATCH();

op_nclip:
	FLAG = 0;

	MAC[0] = mac0_add(ctx, ((uint64_t)SX0 * (uint64_t)SY1) + ((uint64_t)SX1 * (uint64_t)SY2) +
				       ((uint64_t)SX2 * (uint64_t)SY0) - ((uint64_t)SX0 * (uint64_t)SY2) -
				       ((uint64_t)SX1 * (uint64_t)SY0) - ((uint64_t)SX2 * (uint64_t)SY1));

	update_flag(ctx);
	DISPATCH();

op_op:
	FLAG = 0;

	do_op(ctx);

	update_flag(ctx);
	DISPATCH();

op_dpcs:
	FLAG = 0;

	dpc(ctx, RGBC);

	update_flag(ctx);
	DISPATCH();

op_intpl:
	FLAG = 0;

	do_intpl(ctx);

	update_flag(ctx);
	DISPATCH();

op_mvmva:
	FLAG = 0;

	do_mvmva(ctx);

	update_flag(ctx);
	DISPATCH();

op_ncds:
	FLAG = 0;

	ncd(ctx, &V[0]);

	update_flag(ctx);
	DISPATCH();

op_cdp:
	FLAG = 0;

	do_cdp(ctx);

	update_flag(ctx);
	DISPATCH();

op_ncdt:
	FLAG = 0;

	for (size_t i = 0; i < ARRAY_SIZE(V); ++i)
		ncd(ctx, &V[i]);

	update_flag(ctx);
	DISPATCH();

op_nccs:
	FLAG = 0;

	ncc(ctx, &V[0]);
	update_flag(ctx);

	DISPATCH();

op_cc:
	FLAG = 0;

	intpl_bk_lcm(ctx);
	intpl_rgb(ctx);
	color_fifo_push(ctx);

	update_flag(ctx);
	DISPATCH();

op_ncs:
	FLAG = 0;

	nc(ctx, &V[0]);

	update_flag(ctx);
	DISPATCH();

op_nct:
	FLAG = 0;

	for (size_t i = 0; i < ARRAY_SIZE(V); ++i)
		nc(ctx, &V[i]);

	update_flag(ctx);
	DISPATCH();

op_sqr:
	FLAG = 0;

	do_sqr(ctx);

	update_flag(ctx);
	DISPATCH();

op_dpcl:
	FLAG = 0;

	do_dpcl(ctx);

	update_flag(ctx);
	DISPATCH();

op_dpct:
	FLAG = 0;

	for (unsigned int i = 0; i < 3; ++i)
		dpc(ctx, RGB0);

	update_flag(ctx);
	DISPATCH();

op_avsz3:
	avsz(ctx, ZSF3, 1);
	DISPATCH();

op_avsz4:
	avsz(ctx, ZSF4, 0);
	DISPATCH();

op_rtpt:
	FLAG = 0;

	rtp(ctx, &V[0], false);
	rtp(ctx, &V[1], false);
	rtp(ctx, &V[2], true);

	update_flag(ctx);
	DISPATCH();

op_gpf:
	memset(&MAC[1], 0, sizeof(MAC) - sizeof(MAC[0]));

op_gpl:
	FLAG = 0;

	do_gpl(ctx);

	update_flag(ctx);
	DISPATCH();

op_ncct:
	FLAG = 0;

	for (size_t i = 0; i < ARRAY_SIZE(V); ++i)
		ncc(ctx, &V[i]);

	update_flag(ctx);
	DISPATCH();

op_lb:
	do_lb(ctx, m_base, offset, rt);
	DISPATCH();

op_lh:
	do_lh(ctx, m_base, offset, rt);
	DISPATCH();

op_lwl:
	do_lwl(ctx, m_base, offset, rt);
	DISPATCH();

op_lw:
	do_lw(ctx, m_base, offset, rt);
	DISPATCH();

op_lbu:
	do_lbu(ctx, m_base, offset, rt);
	DISPATCH();

op_lhu:
	do_lhu(ctx, m_base, offset, rt);
	DISPATCH();

op_lwr:
	do_lwr(ctx, m_base, offset, rt);
	DISPATCH();

op_sb:
	do_sb(ctx, m_base, offset, rt);
	DISPATCH();

op_sh:
	do_sh(ctx, m_base, offset, rt);
	DISPATCH();

op_swl:
	do_swl(ctx, m_base, offset, rt);
	DISPATCH();

op_sw:
	do_sw(ctx, m_base, offset, rt);
	DISPATCH();

op_swr:
	do_swr(ctx, m_base, offset, rt);
	DISPATCH();

illegal:
	LOG_ERR(ctx, "illegal instruction trapped (pc=0x%08X, instr=0x%08X)", pc, instr);
	ctx->cfg.cpu.illegal_instr(ctx, ctx->cpu.base.instr);

done:
	if (unlikely(trace_pending))
		disasm_trace_emit(ctx);

	return;
}

const struct psycho_cpu_backend_ops psycho_cpu_backend_ops_interpreter = {
	.name  = "Interpreter",
	.run   = run,
	.reset = reset,
};
