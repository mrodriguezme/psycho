// SPDX-License-Identifier: MIT
//
// Copyright 2026 Michael Rodriguez
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <assert.h>
#include <string.h>

#include "bios_trace.h"
#include "bus.h"
#include "cpu_int.h"
#include "cpu_defs.h"
#include "log.h"
#include "util.h"
#include "sched.h"

LOG_MOD(P_LOG_CPU);

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

P_NONNULL static void irq_mux_set(struct p_ctx *ctx, bool set)
{
	if (set) {
		ctx->cpu_int.cop0[P_SR] |= (1 << 10);
		LOG_DBG(ctx, "irq mux line asserted");
	} else {
		ctx->cpu_int.cop0[P_SR] &= ~(1 << 10);
		LOG_DBG(ctx, "irq mux line not asserted");
	}
}

P_NONNULL static void pc_set(struct p_ctx *ctx, u32 pc)
{
	ctx->cpu_int.dly_pc = pc;
	ctx->cpu_int.pc	    = pc;
	ctx->cpu_int.npc    = pc + sizeof(ctx->cpu_int.instr);
}

P_NONNULL static u32 pc_get(struct p_ctx *ctx)
{
	return ctx->cpu_int.pc;
}

P_NONNULL static u32 instr_get(struct p_ctx *ctx)
{
	return ctx->cpu_int.instr;
}

P_NONNULL static void gpr_write_direct(struct p_ctx *ctx, enum p_cpu_gpr gpr, u32 val)
{
	assert(gpr < P_GPR_COUNT);
	ctx->cpu_int.gpr[gpr] = val;
}

P_NONNULL static u32 gpr_read(struct p_ctx *ctx, enum p_cpu_gpr gpr)
{
	assert(gpr < P_GPR_COUNT);
	return ctx->cpu_int.gpr[gpr];
}

P_NONNULL static u32 lo_get(struct p_ctx *ctx)
{
	return ctx->cpu_int.lo;
}

P_NONNULL static u32 hi_get(struct p_ctx *ctx)
{
	return ctx->cpu_int.hi;
}

P_NONNULL static void rst(struct p_ctx *ctx)
{
	memset(ctx->cpu_int.gpr, 0, sizeof(ctx->cpu_int.gpr));
	pc_set(ctx, RST_VECTOR);

	memset(&ctx->cpu_int.ld_pend, 0, sizeof(ctx->cpu_int.ld_pend));
	memset(&ctx->cpu_int.ld_next, 0, sizeof(ctx->cpu_int.ld_next));
	memset(&ctx->cpu_int.cop2, 0, sizeof(ctx->cpu_int.cop2));

	LOG_INFO(ctx, "reset");
}

P_NONNULL static void illegal_instr(struct p_ctx *ctx)
{
	LOG_ERR(ctx, "illegal instruction trapped (pc=0x%08X, instr=0x%08X)",
		ctx->cpu_int.pc, ctx->cpu_int.instr);

	ctx->cfg.cpu.illegal_instr(ctx, ctx->cpu_int.instr);
}

P_NODISCARD P_NONNULL static u32 load32(struct p_ctx *ctx, u32 vaddr)
{
	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return p_load32(ctx, vaddr);
}

P_NODISCARD P_NONNULL static u16 load16(struct p_ctx *ctx, u32 vaddr)
{
	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return p_load16(ctx, vaddr);
}

P_NODISCARD P_NONNULL static u8 load8(struct p_ctx *ctx, u32 vaddr)
{
	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return p_load8(ctx, vaddr);
}

P_NONNULL static void store32(struct p_ctx *ctx, u32 vaddr, u32 data)
{
#define SR (ctx->cpu_int.cop0[P_SR])

	p_sched_adv_ts(ctx, 1);

	if (SR & SR_ISC)
		return;

	vaddr = vaddr_to_paddr(vaddr);
	p_store32(ctx, vaddr, data);

#undef SR
}

P_NONNULL static void store16(struct p_ctx *ctx, u32 vaddr, u16 data)
{
#define SR (ctx->cpu_int.cop0[P_SR])

	if (SR & SR_ISC)
		return;

	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	p_store16(ctx, vaddr, data);

#undef SR
}

P_NONNULL static void store8(struct p_ctx *ctx, u32 vaddr, u8 data)
{
#define SR (ctx->cpu_int.cop0[P_SR])

	if (SR & SR_ISC)
		return;

	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	p_store8(ctx, vaddr, data);

#undef SR
}

P_NONNULL static void gpr_set(struct p_ctx *ctx, size_t reg, u32 val)
{
	// If the instruction following a load writes to the same destination
	// register, the load’s delay slot is canceled.
	if (unlikely(ctx->cpu_int.ld_next.dst == reg))
		memset(&ctx->cpu_int.ld_next, 0, sizeof(ctx->cpu_int.ld_next));

	// Don't bother putting a check for a write to gpr[0] here; it's already
	// bad enough that we have a branch. gpr[0] is unconditionally set to 0
	// at the end of every step.
	ctx->cpu_int.gpr[reg] = val;
}

P_NONNULL static void branch(struct p_ctx *ctx, u32 addr)
{
	ctx->cpu_int.next_in_bd = true;
	ctx->cpu_int.npc	= addr;
}

P_NONNULL static void branch_if(struct p_ctx *ctx, bool cond)
{
	ctx->cpu_int.next_in_bd = true;

	if (cond) {
		u32 pc = unlikely(ctx->cpu_int.in_bd) ?
				 ctx->cpu_int.dly_pc - sizeof(u32) :
				 ctx->cpu_int.pc;

		ctx->cpu_int.npc = branch_addr(pc, ctx->cpu_int.instr);
	}
}

P_NONNULL static void exc(struct p_ctx *ctx, enum cpu_exc exc)
{
#define CAUSE (ctx->cpu_int.cop0[P_CAUSE])
#define EPC   (ctx->cpu_int.cop0[P_EPC])
#define SR    (ctx->cpu_int.cop0[P_SR])

	// So, on an exception, the CPU:

	// 1) sets up EPC to point to the restart location.
	EPC = ctx->cpu_int.pc;

	// 2) the pre-existing user-mode and interrupt-enable flags in SR are
	//    saved by pushing the 3-entry stack inside SR, and changing to
	//    kernel mode with interrupts disabled.
	SR = (SR & ~0x3F) | ((SR << 2) & 0x3F);

	// 3) Cause is setup so that software can see the reason for the
	//    exception.
	//
	//    Clear everything except the IP bits.
	CAUSE = (CAUSE & 0x0000FF00) | (exc << 2);

	// 4) transfers control to the exception entry point.
	pc_set(ctx, 0x80000080);

#undef CAUSE
#undef EPC
#undef SR
}

P_NONNULL static void do_div(struct p_ctx *ctx, s32 dividend, s32 divisor)
{
#define LO (ctx->cpu_int.lo)
#define HI (ctx->cpu_int.hi)

	if (unlikely(!divisor)) {
		// That is, if the dividend is negative, the quotient is 1
		// (0x00000001), and if the dividend is positive or zero, the
		// quotient is -1 (0xFFFFFFFF).
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

P_NONNULL static void do_divu(struct p_ctx *ctx, u32 dividend, u32 divisor)
{
#define LO (ctx->cpu_int.lo)
#define HI (ctx->cpu_int.hi)

	if (unlikely(!divisor)) {
		// In the case of unsigned division, the dividend can't be
		// negative and thus the quotient is always -1 (0xFFFFFFFF) and
		// the remainder equals the dividend.
		LO = UINT32_MAX;
		HI = dividend;
	} else {
		LO = dividend / divisor;
		HI = dividend % divisor;
	}

#undef LO
#undef HI
}

P_NONNULL static void do_add(struct p_ctx *ctx, size_t dst, u32 a0, u32 a1)
{
	int sum;

	if (unlikely(__builtin_sadd_overflow(a0, a1, &sum)))
		exc(ctx, EXC_OV);
	else
		gpr_set(ctx, dst, sum);
}

P_NONNULL static void do_sub(struct p_ctx *ctx, size_t dst, u32 minuend,
			     u32 subtrahend)
{
	int diff;

	if (unlikely(__builtin_ssub_overflow(minuend, subtrahend, &diff)))
		exc(ctx, EXC_OV);
	else
		gpr_set(ctx, dst, diff);
}

P_NONNULL static void do_cop0_instr(struct p_ctx *ctx, uint funct)
{
#define SR (ctx->cpu_int.cop0[P_SR])

	if (unlikely(funct != RFE))
		illegal_instr(ctx);
	else
		SR = (SR & ~0x0F) | ((SR >> 2) & 0x0F);

#undef SR
}

P_NONNULL static void update_flag(struct p_ctx *ctx)
{
#define FLAG (ctx->cpu_int.cop2.ccr.flag)

	if (FLAG & FLAG_ERR_MASK)
		FLAG |= FLAG_ERR;
	else
		FLAG &= ~FLAG_ERR;

#undef FLAG
}

P_NONNULL static void flag_set(struct p_ctx *ctx, u32 flags)
{
#define FLAG (ctx->cpu_int.cop2.ccr.flag)

	FLAG |= flags;

#undef FLAG
}

P_NONNULL static s64 mac123_chk(struct p_ctx *ctx, s64 sum, s64 addend,
				u32 neg_flag, u32 pos_flag)
{
	sum += addend;

	if (sum > MAC123_MAX)
		flag_set(ctx, pos_flag);
	else if (sum < MAC123_MIN)
		flag_set(ctx, neg_flag);

	return (s64)((u64)sum << 20) >> 20;
}

P_NONNULL static s64 mac0_add(struct p_ctx *ctx, s64 res)
{
	if (res > INT32_MAX)
		flag_set(ctx, MAC0_POS_OVF);
	else if (res < INT32_MIN)
		flag_set(ctx, MAC0_NEG_OVF);

	return res;
}

P_NONNULL static s64 mac123_add(struct p_ctx *ctx, size_t mac, s64 sum,
				s64 addend)
{
	switch (mac) {
	case 1:
		return mac123_chk(ctx, sum, addend, MAC1_OVF_NEG, MAC1_OVF_POS);

	case 2:
		return mac123_chk(ctx, sum, addend, MAC2_OVF_NEG, MAC2_OVF_POS);

	case 3:
		return mac123_chk(ctx, sum, addend, MAC3_OVF_NEG, MAC3_OVF_POS);

	default:
		P_UNREACHABLE;
	}
}

P_NONNULL P_NODISCARD static s16 ir0_sat(struct p_ctx *ctx, s64 val)
{
	return gte_clamp(ctx, val, IR0_MIN, IR0_MAX, IR0_SAT);
}

P_NONNULL static s16 ir123_sat(struct p_ctx *ctx, uint ir, s32 val, bool lm)
{
	s16 min = lm ? IR123_LM_MIN : IR123_MIN;

	switch (ir) {
	case 1:
		return gte_clamp(ctx, val, min, IR123_MAX, IR1_SAT);

	case 2:
		return gte_clamp(ctx, val, min, IR123_MAX, IR2_SAT);

	case 3:
		return gte_clamp(ctx, val, min, IR123_MAX, IR3_SAT);

	default:
		P_UNREACHABLE;
	}
}

P_NONNULL static void sx_push(struct p_ctx *ctx, s64 val)
{
#define SXY (ctx->cpu_int.cop2.cpr.sxy)

	for (size_t i = 0; i < ARRAY_SIZE(SXY) - 1; ++i)
		SXY[i].x = SXY[i + 1].x;

	SXY[2].x = gte_clamp(ctx, val, SXY_MIN, SXY_MAX, SX2_SAT);

#undef SXY
}

P_NONNULL static void sy_push(struct p_ctx *ctx, s64 val)
{
#define SXY (ctx->cpu_int.cop2.cpr.sxy)

	for (size_t i = 0; i < ARRAY_SIZE(SXY) - 1; ++i)
		SXY[i].y = SXY[i + 1].y;

	SXY[2].y = gte_clamp(ctx, val, SXY_MIN, SXY_MAX, SY2_SAT);

#undef SXY
}

P_NONNULL static void sz_push(struct p_ctx *ctx, s64 val)
{
#define SZ (ctx->cpu_int.cop2.cpr.sz)

	for (size_t i = 0; i < ARRAY_SIZE(SZ) - 1; ++i)
		SZ[i].v = SZ[i + 1].v;

	SZ[3].v = gte_clamp(ctx, val, SZ_OTZ_MIN, SZ_OTZ_MAX, SZ3_OTZ_SAT);

#undef SZ
}

P_NONNULL static u16 otz_set(struct p_ctx *ctx, s64 val)
{
	return gte_clamp(ctx, val, SZ_OTZ_MIN, SZ_OTZ_MAX, SZ3_OTZ_SAT);
}

P_NONNULL static u32 rgb_set(struct p_ctx *ctx, uint color, s32 val)
{
	switch (color) {
	case 0:
		return gte_clamp(ctx, val, RGB_MIN, RGB_MAX, RGB_R_SAT);

	case 1:
		return gte_clamp(ctx, val, RGB_MIN, RGB_MAX, RGB_G_SAT);

	case 2:
		return gte_clamp(ctx, val, RGB_MIN, RGB_MAX, RGB_B_SAT);

	default:
		P_UNREACHABLE;
	}
}

P_NONNULL P_NODISCARD static s64 gte_div(struct p_ctx *ctx)
{
#define H   (ctx->cpu_int.cop2.ccr.h)
#define SZ3 (ctx->cpu_int.cop2.cpr.sz[3].v)

	s64 n;

	if (likely(H < (SZ3 * 2))) {
		int z  = __builtin_clz(SZ3) - 16;
		n      = H << z;
		uint d = SZ3 << z;
		uint u = unr[(d - 0x7FC0) >> 7] + 0x101;
		d      = ((0x2000080 - (d * u)) >> 8);
		d      = ((0x0000080 + (d * u)) >> 8);
		n      = min(0x1FFFF, (((n * d) + 0x8000) >> 16));
	} else {
		flag_set(ctx, DIV_OVF);
		n = 0x1FFFF;
	}
	return n;

#undef H
#undef SZ3
}

P_NONNULL static void color_fifo_push(struct p_ctx *ctx)
{
#define MAC  (ctx->cpu_int.cop2.cpr.mac)
#define RGB  (ctx->cpu_int.cop2.cpr.rgb)
#define RGBC (ctx->cpu_int.cop2.cpr.rgbc)

	u32 r = rgb_set(ctx, 0, (u32)(MAC[1] >> 4)) << 0;
	u32 g = rgb_set(ctx, 1, (u32)(MAC[2] >> 4)) << 8;
	u32 b = rgb_set(ctx, 2, (u32)(MAC[3] >> 4)) << 16;
	u32 c = (u32)RGBC.code << 24;

	RGB[0]	   = RGB[1];
	RGB[1]	   = RGB[2];
	RGB[2].raw = r | g | b | c;

#undef MAC
#undef RGB
#undef RGBC
}

P_NONNULL static void rtp(struct p_ctx *ctx, struct p_gte_vec *vec, bool dq)
{
#define DQA (ctx->cpu_int.cop2.ccr.dqa)
#define DQB (ctx->cpu_int.cop2.ccr.dqb)
#define IR  (ctx->cpu_int.cop2.cpr.ir)
#define MAC (ctx->cpu_int.cop2.cpr.mac)
#define OFX (ctx->cpu_int.cop2.ccr.ofx)
#define OFY (ctx->cpu_int.cop2.ccr.ofy)
#define RT  (ctx->cpu_int.cop2.ccr.r)
#define TR  (ctx->cpu_int.cop2.ccr.tr)

	const uint sf = shift_frac(ctx->cpu_int.instr);
	const bool lm = ir123_lm(ctx->cpu_int.instr);

	s64 sum;

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		sum = 0;
		sum = mac123_add(ctx, i, sum, (u64)TR[j] << 12);
		sum = mac123_add(ctx, i, sum, RT[j][0] * vec->x);
		sum = mac123_add(ctx, i, sum, RT[j][1] * vec->y);
		sum = mac123_add(ctx, i, sum, RT[j][2] * vec->z);

		// The last iteration of this loop will give us the 44-bit MAC3;
		// we'll need it to handle the special IR3 case
		MAC[i] = sum >> sf;
	}

	sum >>= 12;

	for (size_t i = 1; i < ARRAY_SIZE(MAC) - 1; ++i)
		IR[i] = ir123_sat(ctx, i, MAC[i], lm);

	(void)ir123_sat(ctx, 3, sum, false);
	IR[3] = clamp(MAC[3], lm ? IR123_LM_MIN : IR123_MIN, IR123_MAX);

	sz_push(ctx, sum);

	s64 quot = gte_div(ctx);

	sum = mac0_add(ctx, (quot * IR[1]) + OFX);
	sx_push(ctx, sum >> 16);

	sum = mac0_add(ctx, (quot * IR[2]) + OFY);
	sy_push(ctx, sum >> 16);

	if (dq) {
		sum   = mac0_add(ctx, (quot * DQA) + DQB);
		IR[0] = ir0_sat(ctx, sum >> 12);
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

P_NONNULL static void intpl_llm_vec(struct p_ctx *ctx, struct p_gte_vec *vec)
{
#define IR  (ctx->cpu_int.cop2.cpr.ir)
#define LLM (ctx->cpu_int.cop2.ccr.llm)
#define MAC (ctx->cpu_int.cop2.cpr.mac)

	const uint sf = shift_frac(ctx->cpu_int.instr);
	const bool lm = ir123_lm(ctx->cpu_int.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		s64 sum = 0;
		sum	= mac123_add(ctx, i, sum, LLM[j][0] * vec->x);
		sum	= mac123_add(ctx, i, sum, LLM[j][1] * vec->y);
		sum	= mac123_add(ctx, i, sum, LLM[j][2] * vec->z);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef IR
#undef LLM
#undef MAC
}

P_NONNULL static void intpl_bk_lcm(struct p_ctx *ctx)
{
#define BK  (ctx->cpu_int.cop2.ccr.bk)
#define IR  (ctx->cpu_int.cop2.cpr.ir)
#define LCM (ctx->cpu_int.cop2.ccr.lcm)
#define MAC (ctx->cpu_int.cop2.cpr.mac)

	const uint sf = shift_frac(ctx->cpu_int.instr);
	const bool lm = ir123_lm(ctx->cpu_int.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		s64 sum = 0;
		sum	= mac123_add(ctx, i, sum, (u64)BK[j] << 12);
		sum	= mac123_add(ctx, i, sum, LCM[j][0] * IR[1]);
		sum	= mac123_add(ctx, i, sum, LCM[j][1] * IR[2]);
		sum	= mac123_add(ctx, i, sum, LCM[j][2] * IR[3]);

		MAC[i] = sum >> sf;
	}

	for (size_t i = 1; i < ARRAY_SIZE(MAC); ++i)
		IR[i] = ir123_sat(ctx, i, MAC[i], lm);

#undef BK
#undef IR
#undef LCM
#undef MAC
}

P_NONNULL static void intpl_rgb(struct p_ctx *ctx)
{
#define IR   (ctx->cpu_int.cop2.cpr.ir)
#define MAC  (ctx->cpu_int.cop2.cpr.mac)
#define RGBC (ctx->cpu_int.cop2.cpr.rgbc.arr)

	const uint sf = shift_frac(ctx->cpu_int.instr);
	const bool lm = ir123_lm(ctx->cpu_int.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		MAC[i] = (((u64)(RGBC[j] * IR[i])) << 4) >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef IR
#undef MAC
#undef RGBC
}

P_NONNULL static void intpl_fc(struct p_ctx *ctx, s64 *sums)
{
#define FC  (ctx->cpu_int.cop2.ccr.fc)
#define IR  (ctx->cpu_int.cop2.cpr.ir)
#define MAC (ctx->cpu_int.cop2.cpr.mac)

	const uint sf = shift_frac(ctx->cpu_int.instr);
	const bool lm = ir123_lm(ctx->cpu_int.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		s64 sum = 0;
		sum	= mac123_add(ctx, i, sum, ((u64)FC[j] << 12) - sums[j]);

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

P_NONNULL static void dpc(struct p_ctx *ctx, u8 *rgb)
{
	s64 sums[3];

	for (size_t i = 0; i < ARRAY_SIZE(sums); ++i)
		sums[i] = (u32)rgb[i] << 16;

	intpl_fc(ctx, sums);
	color_fifo_push(ctx);
}

P_NONNULL static void nc(struct p_ctx *ctx, struct p_gte_vec *vec)
{
	intpl_llm_vec(ctx, vec);
	intpl_bk_lcm(ctx);
	color_fifo_push(ctx);
}

P_NONNULL static void ncc(struct p_ctx *ctx, struct p_gte_vec *vec)
{
	intpl_llm_vec(ctx, vec);
	intpl_bk_lcm(ctx);
	intpl_rgb(ctx);
	color_fifo_push(ctx);
}

P_NONNULL static void ncd(struct p_ctx *ctx, struct p_gte_vec *vec)
{
#define IR   (ctx->cpu_int.cop2.cpr.ir)
#define RGBC (ctx->cpu_int.cop2.cpr.rgbc.arr)

	intpl_llm_vec(ctx, vec);
	intpl_bk_lcm(ctx);

	s64 sums[3];

	for (size_t i = 0, j = 1; i < ARRAY_SIZE(sums); ++i, ++j)
		sums[i] = ((u64)(RGBC[i] * IR[j])) << 4;

	intpl_fc(ctx, sums);
	color_fifo_push(ctx);

#undef IR
#undef RGBC
}

P_NONNULL static void avsz(struct p_ctx *ctx, s16 scale, size_t sz_off)
{
#define FLAG (ctx->cpu_int.cop2.ccr.flag)
#define MAC0 (ctx->cpu_int.cop2.cpr.mac[0])
#define OTZ  (ctx->cpu_int.cop2.cpr.otz)
#define SZ   (ctx->cpu_int.cop2.cpr.sz)

	FLAG = 0;

	s64 sum = 0;

	while (sz_off < ARRAY_SIZE(SZ))
		sum += SZ[sz_off++].v;

	sum *= scale;

	MAC0 = mac0_add(ctx, sum);
	OTZ  = otz_set(ctx, sum >> 12);

	update_flag(ctx);

#undef FLAG
#undef MAC0
#undef OTZ
#undef SZ
}

P_NONNULL static void do_cop2_mfc(struct p_ctx *ctx, size_t rt, size_t rd)
{
#define IR   (ctx->cpu_int.cop2.cpr.ir)
#define SXY2 (ctx->cpu_int.cop2.cpr.sxy[2].raw)

	switch (rd) {
	case P_SXYP:
		gpr_set(ctx, rt, SXY2);
		break;

	case P_IRGB:
	case P_ORGB: {
		u32 r = clamp(IR[1] >> 7, 0x00, 0x1F) << 0;
		u32 g = clamp(IR[2] >> 7, 0x00, 0x1F) << 5;
		u32 b = clamp(IR[3] >> 7, 0x00, 0x1F) << 10;

		gpr_set(ctx, rt, b | g | r);
		break;
	}

	default:
		gpr_set(ctx, rt, ctx->cpu_int.cop2.cpr.raw[rd]);
		break;
	}

#undef IR
#undef SXY2
}

P_NONNULL static void do_cop2_cfc(struct p_ctx *ctx, size_t rt, size_t rd)
{
#define H (ctx->cpu_int.cop2.ccr.h)

	if (rd == P_H)
		gpr_set(ctx, rt, sext_16_32(H));
	else
		gpr_set(ctx, rt, ctx->cpu_int.cop2.ccr.raw[rd]);
#undef H
}

P_NONNULL static void do_cop2_mtc(struct p_ctx *ctx, size_t rd, size_t rt)
{
#define gpr  (ctx->cpu_int.gpr)
#define IR   (ctx->cpu_int.cop2.cpr.ir)
#define LZCR (ctx->cpu_int.cop2.cpr.lzcr)
#define LZCS (ctx->cpu_int.cop2.cpr.lzcs)
#define SXY  (ctx->cpu_int.cop2.cpr.sxy)

	switch (rd) {
	case P_OTZ:
	case P_SZ0:
	case P_SZ1:
	case P_SZ2:
	case P_SZ3:
		ctx->cpu_int.cop2.cpr.raw[rd] = zext_16_32(gpr[rt]);
		break;

	case P_VZ0:
	case P_VZ1:
	case P_VZ2:
	case P_IR0:
	case P_IR1:
	case P_IR2:
	case P_IR3:
		ctx->cpu_int.cop2.cpr.raw[rd] = sext_16_32(gpr[rt]);
		break;

	case P_IRGB:
		IR[1] = ((gpr[rt] >> 0) & 0x1F) << 7;
		IR[2] = ((gpr[rt] >> 5) & 0x1F) << 7;
		IR[3] = ((gpr[rt] >> 10) & 0x1F) << 7;

		break;

	case P_SXYP:
		SXY[0]	   = SXY[1];
		SXY[1]	   = SXY[2];
		SXY[2].raw = gpr[rt];

		break;

	case P_LZCS: {
		LZCS = gpr[rt];

		u32 res = 32;

		if (LZCS > 0)
			// Reading LZCR returns the leading 0 count of LZCS if
			// LZCS is positive...
			res = __builtin_clz(LZCS);
		else if (LZCS < 0)
			// and the leading 1 count of LZCS if LZCS is negative.
			res = (LZCS == -1) ? 32 : __builtin_clz(~LZCS);

		// The results are in range 1..32.
		LZCR = res;
		break;
	}

	case P_LZCR:
		// Read-only register - ignore
		break;

	default:
		ctx->cpu_int.cop2.cpr.raw[rd] = gpr[rt];
		break;
	}

#undef gpr
#undef IR
#undef LZCR
#undef LZCS
#undef SXY
}

P_NONNULL static void do_cop2_ctc(struct p_ctx *ctx, size_t rd, size_t rt)
{
#define gpr  (ctx->cpu_int.gpr)
#define FLAG (ctx->cpu_int.cop2.ccr.flag)

	switch (rd) {
	case P_R33:
	case P_L33:
	case P_DQA:
	case P_LB3:
	case P_ZSF3:
	case P_ZSF4:
		ctx->cpu_int.cop2.ccr.raw[rd] = sext_16_32(gpr[rt]);
		break;

	case P_FLAG:
		FLAG = gpr[rt] & FLAG_MASK;
		update_flag(ctx);

		break;

	default:
		ctx->cpu_int.cop2.ccr.raw[rd] = gpr[rt];
		break;
	}

#undef gpr
#undef FLAG
}

static void mvmva(struct p_ctx *ctx, s16 (*Mx)[3], s16 *Vx, s32 *Tx)
{
#define IR  (ctx->cpu_int.cop2.cpr.ir)
#define MAC (ctx->cpu_int.cop2.cpr.mac)

	const uint sf = shift_frac(ctx->cpu_int.instr);
	const bool lm = ir123_lm(ctx->cpu_int.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		s64 sum = 0;
		sum	= mac123_add(ctx, i, sum, (u64)Tx[j] << 12);
		sum	= mac123_add(ctx, i, sum, Mx[j][0] * Vx[0]);
		sum	= mac123_add(ctx, i, sum, Mx[j][1] * Vx[1]);
		sum	= mac123_add(ctx, i, sum, Mx[j][2] * Vx[2]);

		MAC[i] = sum >> sf;
		IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
	}

#undef IR
#undef MAC
}

static void mvmva_bugged(struct p_ctx *ctx, s16 (*Mx)[3], s16 *Vx)
{
#define FC  (ctx->cpu_int.cop2.ccr.fc)
#define IR  (ctx->cpu_int.cop2.cpr.ir)
#define MAC (ctx->cpu_int.cop2.cpr.mac)

	const uint sf = shift_frac(ctx->cpu_int.instr);
	const bool lm = ir123_lm(ctx->cpu_int.instr);

	for (size_t i = 1, j = 0; i < ARRAY_SIZE(MAC); ++i, ++j) {
		s64 sum = 0;
		sum	= mac123_add(ctx, i, sum, (u64)FC[j] << 12);
		sum	= mac123_add(ctx, i, sum, Mx[j][0] * Vx[0]);

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

P_NONNULL static void do_cop2_instr(struct p_ctx *ctx, uint funct)
{
#define BK   (ctx->cpu_int.cop2.ccr.bk)
#define D1   (ctx->cpu_int.cop2.ccr.r[0][0])
#define D2   (ctx->cpu_int.cop2.ccr.r[1][1])
#define D3   (ctx->cpu_int.cop2.ccr.r[2][2])
#define FC   (ctx->cpu_int.cop2.ccr.fc)
#define FLAG (ctx->cpu_int.cop2.ccr.flag)
#define IR   (ctx->cpu_int.cop2.cpr.ir)
#define LCM  (ctx->cpu_int.cop2.ccr.lcm)
#define LLM  (ctx->cpu_int.cop2.ccr.llm)
#define MAC  (ctx->cpu_int.cop2.cpr.mac)
#define RGB0 (ctx->cpu_int.cop2.cpr.rgb[0].arr)
#define RGBC (ctx->cpu_int.cop2.cpr.rgbc.arr)
#define RT   (ctx->cpu_int.cop2.ccr.r)
#define SX0  (ctx->cpu_int.cop2.cpr.sxy[0].x)
#define SX1  (ctx->cpu_int.cop2.cpr.sxy[1].x)
#define SX2  (ctx->cpu_int.cop2.cpr.sxy[2].x)
#define SY0  (ctx->cpu_int.cop2.cpr.sxy[0].y)
#define SY1  (ctx->cpu_int.cop2.cpr.sxy[1].y)
#define SY2  (ctx->cpu_int.cop2.cpr.sxy[2].y)
#define TR   (ctx->cpu_int.cop2.ccr.tr)
#define V    (ctx->cpu_int.cop2.cpr.v)
#define ZSF3 (ctx->cpu_int.cop2.ccr.zsf3)
#define ZSF4 (ctx->cpu_int.cop2.ccr.zsf4)

	switch (funct) {
	case RTPS:
		FLAG = 0;

		rtp(ctx, &V[0], true);
		update_flag(ctx);

		return;

	case NCLIP:
		FLAG = 0;

		MAC[0] = mac0_add(
			ctx,
			((u64)SX0 * (u64)SY1) + ((u64)SX1 * (u64)SY2) +
				((u64)SX2 * (u64)SY0) - ((u64)SX0 * (u64)SY2) -
				((u64)SX1 * (u64)SY0) - ((u64)SX2 * (u64)SY1));

		update_flag(ctx);
		return;

	case OP: {
		FLAG = 0;

		const uint sf = shift_frac(ctx->cpu_int.instr);
		const bool lm = ir123_lm(ctx->cpu_int.instr);

		s64 res[3] = { [0] = (IR[3] * D2) - (IR[2] * D3),
			       [1] = (IR[1] * D3) - (IR[3] * D1),
			       [2] = (IR[2] * D1) - (IR[1] * D2) };

		for (size_t i = 0, reg = 1; i < ARRAY_SIZE(res); ++i, ++reg) {
			s64 sum = 0;
			sum	= mac123_add(ctx, reg, sum, res[i]);

			MAC[reg] = sum >> sf;
			IR[reg]	 = ir123_sat(ctx, reg, MAC[reg], lm);
		}

		update_flag(ctx);
		return;
	}

	case DPCS:
		FLAG = 0;

		dpc(ctx, RGBC);

		update_flag(ctx);
		return;

	case INTPL: {
		FLAG = 0;

		s64 sums[3];

		for (size_t i = 0, j = 1; i < ARRAY_SIZE(sums); ++i, ++j)
			sums[i] = (u64)IR[j] << 12;

		intpl_fc(ctx, sums);
		color_fifo_push(ctx);

		update_flag(ctx);
		return;
	}

	case MVMVA: {
		FLAG = 0;

		s16(*mx_lut[3])[3] = {
			// clang-format off

			[0] = RT,
			[1] = LLM,
			[2] = LCM

			// clang-format on
		};

		s32 *tx_lut[4] = {
			// clang-format off

			[0] = TR,
			[1] = BK,
			[2] = FC,
			[3] = (s32[3]){ 0 }

			// clang-format on
		};

		uint mx_sel = mvmva_mx(ctx->cpu_int.instr);

		s16(*mx)[3];
		s16 mx_bugged[3][3];

		if (likely(mx_sel <= 2))
			mx = mx_lut[mx_sel];
		else {
			mx_bugged[0][0] = (u32)-RGBC[0] << 4;
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

		uint tx_sel = mvmva_tx(ctx->cpu_int.instr);
		s32 *tx	    = tx_lut[tx_sel];

		uint vx_sel = mvmva_vx(ctx->cpu_int.instr);

		s16 vx_tmp[3];
		s16 *vx;

		if (vx_sel <= 2)
			vx = V[vx_sel].arr;
		else {
			vx_tmp[0] = IR[1];
			vx_tmp[1] = IR[2];
			vx_tmp[2] = IR[3];
			vx	  = vx_tmp;
		}

		if (tx_sel != 2)
			mvmva(ctx, mx, vx, tx);
		else
			// Don't need to worry about passing Tx - only the FC
			// vector is buggy
			mvmva_bugged(ctx, mx, vx);

		update_flag(ctx);
		return;
	}

	case NCDS:
		FLAG = 0;

		ncd(ctx, &V[0]);

		update_flag(ctx);
		return;

	case CDP: {
		FLAG = 0;

		intpl_bk_lcm(ctx);

		s64 sums[3];

		for (size_t i = 0, j = 1; i < ARRAY_SIZE(sums); ++i, ++j)
			sums[i] = ((u64)(RGBC[i] * IR[j])) << 4;

		intpl_fc(ctx, sums);
		color_fifo_push(ctx);

		update_flag(ctx);
		return;
	}

	case NCDT:
		FLAG = 0;

		for (size_t i = 0; i < ARRAY_SIZE(V); ++i)
			ncd(ctx, &V[i]);

		update_flag(ctx);
		return;

	case NCCS:
		FLAG = 0;

		ncc(ctx, &V[0]);

		update_flag(ctx);
		return;

	case CC:
		FLAG = 0;

		intpl_bk_lcm(ctx);
		intpl_rgb(ctx);
		color_fifo_push(ctx);

		update_flag(ctx);
		return;

	case NCS:
		FLAG = 0;

		nc(ctx, &V[0]);

		update_flag(ctx);
		return;

	case NCT:
		FLAG = 0;

		for (size_t i = 0; i < ARRAY_SIZE(V); ++i)
			nc(ctx, &V[i]);

		update_flag(ctx);
		return;

	case SQR: {
		FLAG = 0;

		const uint sf = shift_frac(ctx->cpu_int.instr);
		const bool lm = ir123_lm(ctx->cpu_int.instr);

		for (size_t i = 1; i < ARRAY_SIZE(IR); ++i) {
			MAC[i] = ((s64)IR[i] * (s64)IR[i]) >> sf;
			IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
		}

		update_flag(ctx);
		return;
	}

	case DPCL: {
		FLAG = 0;

		s64 sums[3];

		for (size_t i = 0, j = 1; i < ARRAY_SIZE(sums); ++i, ++j)
			sums[i] = ((u64)(RGBC[i] * IR[j])) << 4;

		intpl_fc(ctx, sums);
		color_fifo_push(ctx);

		update_flag(ctx);
		return;
	}

	case DPCT:
		FLAG = 0;

		for (uint i = 0; i < 3; ++i)
			dpc(ctx, RGB0);

		update_flag(ctx);
		return;

	case AVSZ3:
		avsz(ctx, ZSF3, 1);
		return;

	case AVSZ4:
		avsz(ctx, ZSF4, 0);
		return;

	case RTPT:
		FLAG = 0;

		rtp(ctx, &V[0], false);
		rtp(ctx, &V[1], false);
		rtp(ctx, &V[2], true);

		update_flag(ctx);
		return;

	case GPF:
		memset(&MAC[1], 0, sizeof(MAC) - 1);
		P_FALLTHROUGH;

	case GPL: {
		FLAG = 0;

		const uint sf = shift_frac(ctx->cpu_int.instr);
		const bool lm = ir123_lm(ctx->cpu_int.instr);

		for (size_t i = 1; i < ARRAY_SIZE(MAC); ++i) {
			s64 sum = 0;
			sum	= mac123_add(ctx, i, sum, (u64)MAC[i] << sf);
			sum	= mac123_add(ctx, i, sum, IR[i] * IR[0]);

			MAC[i] = sum >> sf;
			IR[i]  = ir123_sat(ctx, i, MAC[i], lm);
		}

		color_fifo_push(ctx);
		update_flag(ctx);

		return;
	}

	case NCCT:
		FLAG = 0;

		for (size_t i = 0; i < ARRAY_SIZE(V); ++i)
			ncc(ctx, &V[i]);

		update_flag(ctx);
		return;

	default:
		illegal_instr(ctx);
		return;
	}

#undef BK
#undef D1
#undef D2
#undef D3
#undef FC
#undef FLAG
#undef IR
#undef LCM
#undef LLM
#undef MAC
#undef RGB0
#undef RGBC
#undef RT
#undef SX0
#undef SX1
#undef SX2
#undef SY0
#undef SY1
#undef SY2
#undef TR
#undef V
#undef ZSF3
#undef ZSF4
}

P_NONNULL static void dly_slot_process(struct p_ctx *ctx)
{
	ctx->cpu_int.gpr[ctx->cpu_int.ld_next.dst] = ctx->cpu_int.ld_next.val;

	if (ctx->cpu_int.ld_next.dst)
		LOG_TRACE(ctx, "load delay eviction: %zu <- 0x%08X",
			  ctx->cpu_int.ld_next.dst, ctx->cpu_int.ld_next.val);

	memset(&ctx->cpu_int.ld_next, 0, sizeof(ctx->cpu_int.ld_next));
	swap(&ctx->cpu_int.ld_pend, &ctx->cpu_int.ld_next);
}

P_NONNULL static void load_dly(struct p_ctx *ctx, size_t dst, u32 val)
{
	if (unlikely(!dst)) {
		LOG_DBG(ctx, "Load delay rejected - dest was $zero");
		return;
	}

	ctx->cpu_int.ld_pend.dst = dst;
	ctx->cpu_int.ld_pend.val = val;

	LOG_TRACE(ctx, "load delay pending: dst=%zu, val=0x%08X", dst, val);

	if (unlikely(ctx->cpu_int.ld_next.dst == dst))
		memset(&ctx->cpu_int.ld_next, 0, sizeof(ctx->cpu_int.ld_next));
}

P_NONNULL static void step(struct p_ctx *ctx)
{
#define gpr	(ctx->cpu_int.gpr)
#define pc	(ctx->cpu_int.pc)
#define npc	(ctx->cpu_int.npc)
#define hi	(ctx->cpu_int.hi)
#define lo	(ctx->cpu_int.lo)
#define instr	(ctx->cpu_int.instr)

#define op	(instr_op(instr))
#define rt	(instr_rt(instr))
#define rs	(instr_rs(instr))
#define rd	(instr_rd(instr))
#define shamt	(instr_shamt(instr))
#define funct	(instr_funct(instr))
#define base	(rs)
#define zextimm (zext_16_32(instr_imm(instr)))
#define sextimm (sext_16_32(instr_imm(instr)))
#define offset	(sextimm)

	if (unlikely(ctx->cpu_int.dly_pc & 3))
		exc(ctx, EXC_ADEL);

	ctx->cpu_int.in_bd	= ctx->cpu_int.next_in_bd;
	ctx->cpu_int.next_in_bd = false;

	pc    = ctx->cpu_int.dly_pc;
	instr = load32(ctx, pc);

	ctx->cpu_int.dly_pc = npc;
	npc		    = ctx->cpu_int.dly_pc + sizeof(instr);

	dly_slot_process(ctx);

	switch (op) {
	case GRP_SPECIAL:
		switch (funct) {
		case SLL:
			gpr_set(ctx, rd, gpr[rt] << shamt);
			break;

		case SRL:
			gpr_set(ctx, rd, gpr[rt] >> shamt);
			break;

		case SRA:
			gpr_set(ctx, rd, (s32)gpr[rt] >> shamt);
			break;

		case SLLV:
			gpr_set(ctx, rd, gpr[rt] << (gpr[rs] & 0x1F));
			break;

		case SRLV:
			gpr_set(ctx, rd, gpr[rt] >> (gpr[rs] & 0x1F));
			break;

		case SRAV:
			gpr_set(ctx, rd, (s32)gpr[rt] >> (gpr[rs] & 0x1F));
			break;

		case JR:
			branch(ctx, gpr[rs]);
			break;

		case JALR: {
			u32 jmp_addr = gpr[rs];

			gpr_set(ctx, rd, pc + (sizeof(instr) * 2));
			branch(ctx, jmp_addr);

			break;
		}

		case SYSCALL:
			exc(ctx, EXC_SYSCALL);
			break;

		case BREAK:
			exc(ctx, EXC_BP);
			break;

		case MFHI:
			gpr_set(ctx, rd, hi);
			break;

		case MTHI:
			hi = gpr[rs];
			break;

		case MFLO:
			gpr_set(ctx, rd, lo);
			break;

		case MTLO:
			lo = gpr[rs];
			break;

		case MULT: {
			u64 x = sext_32_64(gpr[rs]) * sext_32_64(gpr[rt]);

			lo = x & UINT32_MAX;
			hi = x >> 32;

			break;
		}

		case MULTU: {
			u64 x = zext_32_64(gpr[rs]) * zext_32_64(gpr[rt]);

			lo = x & UINT32_MAX;
			hi = x >> 32;

			break;
		}

		case DIV:
			do_div(ctx, gpr[rs], gpr[rt]);
			break;

		case DIVU:
			do_divu(ctx, gpr[rs], gpr[rt]);
			break;

		case ADD:
			do_add(ctx, rd, gpr[rs], gpr[rt]);
			break;

		case ADDU:
			gpr_set(ctx, rd, gpr[rs] + gpr[rt]);
			break;

		case SUB:
			do_sub(ctx, rd, gpr[rs], gpr[rt]);
			break;

		case SUBU:
			gpr_set(ctx, rd, gpr[rs] - gpr[rt]);
			break;

		case AND:
			gpr_set(ctx, rd, gpr[rs] & gpr[rt]);
			break;

		case OR:
			gpr_set(ctx, rd, gpr[rs] | gpr[rt]);
			break;

		case XOR:
			gpr_set(ctx, rd, gpr[rs] ^ gpr[rt]);
			break;

		case NOR:
			gpr_set(ctx, rd, ~(gpr[rs] | gpr[rt]));
			break;

		case SLT:
			gpr_set(ctx, rd, (s32)gpr[rs] < (s32)gpr[rt]);
			break;

		case SLTU:
			gpr_set(ctx, rd, gpr[rs] < gpr[rt]);
			break;

		default:
			illegal_instr(ctx);
			break;
		}
		break;

	case GRP_REGIMM: {
		bool link   = (rt & 0x1E) == 0x10;
		bool branch = (s32)(gpr[rs] ^ (rt << 31)) < 0;

		if (link)
			gpr_set(ctx, P_RA, pc + (sizeof(instr) * 2));

		branch_if(ctx, branch);
		break;
	}

	case J:
		branch(ctx, jmp_addr(pc, instr));
		break;

	case JAL:
		gpr_set(ctx, P_RA, pc + (sizeof(instr) * 2));
		branch(ctx, jmp_addr(pc, instr));

		break;

	case BEQ:
		branch_if(ctx, gpr[rs] == gpr[rt]);
		break;

	case BNE:
		branch_if(ctx, gpr[rs] != gpr[rt]);
		break;

	case BLEZ:
		branch_if(ctx, (s32)gpr[rs] <= 0);
		break;

	case BGTZ:
		branch_if(ctx, (s32)gpr[rs] > 0);
		break;

	case ADDI: {
		int sum;

		if (unlikely(__builtin_sadd_overflow(gpr[rs], sextimm, &sum)))
			exc(ctx, EXC_OV);
		else
			gpr_set(ctx, rt, sum);

		break;
	}

	case ADDIU:
		gpr_set(ctx, rt, gpr[rs] + sextimm);
		break;

	case SLTI:
		gpr_set(ctx, rt, (s32)gpr[rs] < (s32)sextimm);
		break;

	case SLTIU:
		gpr_set(ctx, rt, gpr[rs] < sextimm);
		break;

	case ANDI:
		gpr_set(ctx, rt, zextimm & gpr[rs]);
		break;

	case ORI:
		gpr_set(ctx, rt, zextimm | gpr[rs]);
		break;

	case XORI:
		gpr_set(ctx, rt, zextimm ^ gpr[rs]);
		break;

	case LUI:
		gpr_set(ctx, rt, zextimm << 16);
		break;

	case GRP_COP0:
		switch (rs) {
		case MFC:
			gpr_set(ctx, rt, ctx->cpu_int.cop0[rd]);
			break;

		case MTC:
			ctx->cpu_int.cop0[rd] = gpr[rt];
			break;

		default:
			do_cop0_instr(ctx, funct);
			break;
		}
		break;

	case GRP_COP2:
		switch (rs) {
		case MFC:
			do_cop2_mfc(ctx, rt, rd);
			break;

		case CFC:
			do_cop2_cfc(ctx, rt, rd);
			break;

		case MTC:
			do_cop2_mtc(ctx, rd, rt);
			break;

		case CTC:
			do_cop2_ctc(ctx, rd, rt);
			break;

		default:
			do_cop2_instr(ctx, funct);
			break;
		}
		break;

	case LB:
		load_dly(ctx, rt, sext_8_32(load8(ctx, gpr[base] + offset)));
		break;

	case LH: {
		u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 1)) {
			exc(ctx, EXC_ADEL);
			break;
		}
		load_dly(ctx, rt, sext_16_32(load16(ctx, vaddr)));
		break;
	}

	case LWL: {
		u32 vaddr	  = gpr[base] + offset;
		u32 aligned_vaddr = vaddr & ~3;

		u32 word = load32(ctx, aligned_vaddr);

		uint shift = (vaddr & 3) * 8;
		uint mask  = 0x00FFFFFF >> shift;

		u32 val = (ctx->cpu_int.ld_next.dst == rt) ?
				  ctx->cpu_int.ld_next.val :
				  gpr[rt];

		val = (val & mask) | (word << (24 - shift));
		load_dly(ctx, rt, val);

		break;
	}

	case LW: {
		u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 0x3)) {
			exc(ctx, EXC_ADEL);
			break;
		}
		load_dly(ctx, rt, load32(ctx, vaddr));
		break;
	}

	case LBU:
		load_dly(ctx, rt, zext_8_32(load8(ctx, gpr[base] + offset)));
		break;

	case LHU: {
		u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 1)) {
			exc(ctx, EXC_ADEL);
			break;
		}

		load_dly(ctx, rt, zext_16_32(load16(ctx, vaddr)));
		break;
	}

	case LWR: {
		u32 vaddr	  = gpr[base] + offset;
		u32 aligned_vaddr = vaddr & ~3;

		u32 word = load32(ctx, aligned_vaddr);

		uint shift = (vaddr & 3) * 8;
		uint mask  = 0xFFFFFF00 << (24 - shift);

		u32 val = (ctx->cpu_int.ld_next.dst == rt) ?
				  ctx->cpu_int.ld_next.val :
				  gpr[rt];

		val = (val & mask) | (word >> shift);

		load_dly(ctx, rt, val);
		break;
	}

	case SB:
		store8(ctx, gpr[base] + offset, gpr[rt] & UINT8_MAX);
		break;

	case SH: {
		u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 1)) {
			exc(ctx, EXC_ADES);
			break;
		}

		store16(ctx, vaddr, gpr[rt] & UINT16_MAX);
		break;
	}

	case SWL: {
		u32 vaddr	  = gpr[base] + offset;
		u32 aligned_vaddr = vaddr & ~3;

		uint shift = (vaddr & 3) * 8;
		uint mask  = 0xFFFFFF00 << shift;

		u32 word = load32(ctx, aligned_vaddr);
		word	 = (word & mask) | (gpr[rt] >> (24 - shift));
		store32(ctx, aligned_vaddr, word);

		break;
	}

	case SW: {
		u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 3)) {
			exc(ctx, EXC_ADES);
			break;
		}

		store32(ctx, vaddr, gpr[rt]);
		break;
	}

	case SWR: {
		u32 vaddr	  = gpr[base] + offset;
		u32 aligned_vaddr = vaddr & ~3;

		uint shift = (vaddr & 3) * 8;
		uint mask  = 0x00FFFFFF >> (24 - shift);

		u32 word = load32(ctx, aligned_vaddr);
		word	 = (word & mask) | (gpr[rt] << shift);
		store32(ctx, aligned_vaddr, word);

		break;
	}

	default:
		illegal_instr(ctx);
		break;
	}

	// Better than a branch - ensure that zero is indeed always zero.
	gpr[P_ZERO] = 0;

#undef gpr
#undef pc
#undef npc
#undef hi
#undef lo
#undef instr

#undef op
#undef rt
#undef rs
#undef rd
#undef shamt
#undef funct
#undef base
#undef zextimm
#undef sextimm
#undef offset
}

static void run(struct p_ctx *ctx, u64 cycles, u64 instr_limit,
		bool end_on_event)
{
	cycles += ctx->sched.ts_now;

	while (ctx->sched.ts_now < cycles) {
		p_bios_trace_begin(ctx);
		step(ctx);
		p_bios_trace_end(ctx);
	}
}

void p_cpu_int_init(struct p_ctx *ctx)
{
	ctx->cpu.irq_mux_set = irq_mux_set;

	ctx->cpu.gpr_set = gpr_write_direct;
	ctx->cpu.gpr_get = gpr_read;

	ctx->cpu.lo_get = lo_get;
	ctx->cpu.hi_get = hi_get;

	ctx->cpu.pc_set = pc_set;
	ctx->cpu.pc_get = pc_get;

	ctx->cpu.run = run;
	ctx->cpu.rst = rst;

	ctx->cpu.instr_get = instr_get;
}
