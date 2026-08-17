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
#include "cpu.h"
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

static const u8 unr[0x101] = {
	0xFF, 0xFD, 0xFB, 0xF9, 0xF7, 0xF5, 0xF3, 0xF1, 0xEF, 0xEE, 0xEC, 0xEA,
	0xE8, 0xE6, 0xE4, 0xE3, 0xE1, 0xDF, 0xDD, 0xDC, 0xDA, 0xD8, 0xD6, 0xD5,
	0xD3, 0xD1, 0xD0, 0xCE, 0xCD, 0xCB, 0xC9, 0xC8, 0xC6, 0xC5, 0xC3, 0xC1,
	0xC0, 0xBE, 0xBD, 0xBB, 0xBA, 0xB8, 0xB7, 0xB5, 0xB4, 0xB2, 0xB1, 0xB0,
	0xAE, 0xAD, 0xAB, 0xAA, 0xA9, 0xA7, 0xA6, 0xA4, 0xA3, 0xA2, 0xA0, 0x9F,
	0x9E, 0x9C, 0x9B, 0x9A, 0x99, 0x97, 0x96, 0x95, 0x94, 0x92, 0x91, 0x90,
	0x8F, 0x8D, 0x8C, 0x8B, 0x8A, 0x89, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82,
	0x81, 0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78, 0x77, 0x75, 0x74,
	0x73, 0x72, 0x71, 0x70, 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x69, 0x68,
	0x67, 0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5F, 0x5E, 0x5D, 0x5D,
	0x5C, 0x5B, 0x5A, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53, 0x53, 0x52,
	0x51, 0x50, 0x4F, 0x4E, 0x4D, 0x4D, 0x4C, 0x4B, 0x4A, 0x49, 0x48, 0x48,
	0x47, 0x46, 0x45, 0x44, 0x43, 0x43, 0x42, 0x41, 0x40, 0x3F, 0x3F, 0x3E,
	0x3D, 0x3C, 0x3C, 0x3B, 0x3A, 0x39, 0x39, 0x38, 0x37, 0x36, 0x36, 0x35,
	0x34, 0x33, 0x33, 0x32, 0x31, 0x31, 0x30, 0x2F, 0x2E, 0x2E, 0x2D, 0x2C,
	0x2C, 0x2B, 0x2A, 0x2A, 0x29, 0x28, 0x28, 0x27, 0x26, 0x26, 0x25, 0x24,
	0x24, 0x23, 0x22, 0x22, 0x21, 0x20, 0x20, 0x1F, 0x1E, 0x1E, 0x1D, 0x1D,
	0x1C, 0x1B, 0x1B, 0x1A, 0x19, 0x19, 0x18, 0x18, 0x17, 0x16, 0x16, 0x15,
	0x15, 0x14, 0x14, 0x13, 0x12, 0x12, 0x11, 0x11, 0x10, 0x0F, 0x0F, 0x0E,
	0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x08, 0x08,
	0x07, 0x07, 0x06, 0x06, 0x05, 0x05, 0x04, 0x04, 0x03, 0x03, 0x02, 0x02,
	0x01, 0x01, 0x00, 0x00, 0x00,
};

P_NONNULL static void illegal_instr(struct p_ctx *ctx)
{
	LOG_ERR(ctx, "illegal instruction trapped (pc=0x%08X, instr=0x%08X)",
		ctx->cpu.pc, ctx->cpu.instr);

	ctx->cfg.cpu.illegal_instr(ctx, ctx->cpu.instr);
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
	p_sched_adv_ts(ctx, 1);

	if (ctx->cpu.cop0[P_SR] & SR_ISC)
		return;

	vaddr = vaddr_to_paddr(vaddr);
	p_store32(ctx, vaddr, data);
}

P_NONNULL static void store16(struct p_ctx *ctx, u32 vaddr, u16 data)
{
	if (ctx->cpu.cop0[P_SR] & SR_ISC)
		return;

	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	p_store16(ctx, vaddr, data);
}

P_NONNULL static void store8(struct p_ctx *ctx, u32 vaddr, u8 data)
{
	if (ctx->cpu.cop0[P_SR] & SR_ISC)
		return;

	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	p_store8(ctx, vaddr, data);
}

P_NONNULL static void gpr_set(struct p_ctx *ctx, size_t reg, u32 val)
{
	// If the instruction following a load writes to the same destination
	// register, the load’s delay slot is canceled.
	if (unlikely(ctx->cpu.ld_next.dst == reg))
		memset(&ctx->cpu.ld_next, 0, sizeof(ctx->cpu.ld_next));

	// Don't bother putting a check for a write to gpr[0] here; it's already
	// bad enough that we have a branch. gpr[0] is unconditionally set to 0
	// at the end of every step.
	ctx->cpu.gpr[reg] = val;
}

P_NONNULL static void branch(struct p_ctx *ctx, u32 addr)
{
	ctx->cpu.next_in_bd = true;
	ctx->cpu.npc	    = addr;
}

P_NONNULL static void branch_if(struct p_ctx *ctx, bool cond)
{
	ctx->cpu.next_in_bd = true;

	if (cond) {
		u32 pc = unlikely(ctx->cpu.in_bd) ?
				 ctx->cpu.dly_pc - sizeof(u32) :
				 ctx->cpu.pc;

		ctx->cpu.npc = branch_addr(pc, ctx->cpu.instr);
	}
}

P_NONNULL static void exc(struct p_ctx *ctx, enum cpu_exc exc)
{
#define SR    (ctx->cpu.cop0[P_SR])
#define CAUSE (ctx->cpu.cop0[P_CAUSE])
#define EPC   (ctx->cpu.cop0[P_EPC])

	// So, on an exception, the CPU:

	// 1) sets up EPC to point to the restart location.
	EPC = ctx->cpu.pc;

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
	p_cpu_pc_set(ctx, 0x80000080);

#undef SR
#undef CAUSE
#undef EPC
}

P_NONNULL static void do_div(struct p_ctx *ctx, s32 dividend, s32 divisor)
{
#define LO (ctx->cpu.lo)
#define HI (ctx->cpu.hi)

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
#define LO (ctx->cpu.lo)
#define HI (ctx->cpu.hi)

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
#define SR (ctx->cpu.cop0[P_SR])

	if (unlikely(funct != RFE))
		illegal_instr(ctx);
	else
		SR = (SR & ~0x0F) | ((SR >> 2) & 0x0F);

#undef SR
}

P_NONNULL static void update_flag(struct p_ctx *ctx)
{
	if (ctx->cpu.cop2.ccr.flag & GTE_ERR_FLAG_MASK)
		ctx->cpu.cop2.ccr.flag |= GTE_FLAG_ERR;
	else
		ctx->cpu.cop2.ccr.flag &= ~GTE_FLAG_ERR;
}

P_NONNULL static void flag_set(struct p_ctx *ctx, u32 flags)
{
	ctx->cpu.cop2.ccr.flag |= flags;
}

P_NONNULL static s64 mac123_chk(struct p_ctx *ctx, s64 sum, s64 addend,
				u32 neg_flag, u32 pos_flag)
{
	sum += addend;

	if (sum > GTE_MAC123_MAX)
		flag_set(ctx, pos_flag);
	else if (sum < GTE_MAC123_MIN)
		flag_set(ctx, neg_flag);

	return ((s64)(sum << (64 - 44))) >> (64 - 44);
}

P_NONNULL static s64 mac0_add(struct p_ctx *ctx, s64 res)
{
	if (res > INT32_MAX)
		flag_set(ctx, GTE_FLAG_MAC0_POS_OVF);
	else if (res < INT32_MIN)
		flag_set(ctx, GTE_FLAG_MAC0_NEG_OVF);

	return res;
}

P_NONNULL static s64 mac123_add(struct p_ctx *ctx, uint mac, s64 sum,
				s64 addend)
{
	switch (mac) {
	case 1:
		return mac123_chk(ctx, sum, addend, GTE_FLAG_MAC1_OVF_NEG,
				  GTE_FLAG_MAC1_OVF_POS);

	case 2:
		return mac123_chk(ctx, sum, addend, GTE_FLAG_MAC2_OVF_NEG,
				  GTE_FLAG_MAC2_OVF_POS);

	case 3:
		return mac123_chk(ctx, sum, addend, GTE_FLAG_MAC3_OVF_NEG,
				  GTE_FLAG_MAC3_OVF_POS);

	default:
		__builtin_unreachable();
	}
}

P_NONNULL P_NODISCARD static s16 ir0_sat(struct p_ctx *ctx, s64 val)
{
	return gte_clamp(ctx, val, +0x0000, +0x1000, GTE_FLAG_IR0_SAT);
}

P_NONNULL static s16 ir123_sat(struct p_ctx *ctx, uint ir, s64 val, bool lm)
{
	s16 min = lm ? GTE_IR123_LM_MIN : GTE_IR123_MIN;

	switch (ir) {
	case 1:
		return gte_clamp(ctx, val, min, GTE_IR123_MAX,
				 GTE_FLAG_IR1_SAT);

	case 2:
		return gte_clamp(ctx, val, min, GTE_IR123_MAX,
				 GTE_FLAG_IR2_SAT);

	case 3:
		return gte_clamp(ctx, val, min, GTE_IR123_MAX,
				 GTE_FLAG_IR3_SAT);

	default:
		__builtin_unreachable();
	}
}

P_NODISCARD static uint shift_frac(const uint instr)
{
	return (instr & (1 << 19)) ? 12 : 0;
}

P_NODISCARD static bool ir123_lm(const uint instr)
{
	return instr & (1 << 10);
}

P_NONNULL static void sx_push(struct p_ctx *ctx, s64 val)
{
#define SXY (ctx->cpu.cop2.cpr.sxy)

	for (size_t i = 0; i < ARRAY_SIZE(SXY) - 2; ++i)
		SXY[i].x = SXY[i + 1].x;

	SXY[2].x = gte_clamp(ctx, val, -0x0400, +0x03FF, GTE_FLAG_SX2_SAT);
	SXY[3].x = SXY[2].x;

#undef SXY
}

P_NONNULL static void sy_push(struct p_ctx *ctx, s64 val)
{
#define SXY (ctx->cpu.cop2.cpr.sxy)

	for (size_t i = 0; i < ARRAY_SIZE(SXY) - 2; ++i)
		SXY[i].y = SXY[i + 1].y;

	SXY[2].y = gte_clamp(ctx, val, -0x0400, +0x03FF, GTE_FLAG_SY2_SAT);
	SXY[3].y = SXY[2].y;

#undef SXY
}

P_NONNULL static void sz_push(struct p_ctx *ctx, s64 val)
{
#define SZ (ctx->cpu.cop2.cpr.sz)

	for (size_t i = 0; i < ARRAY_SIZE(SZ) - 1; ++i)
		SZ[i].v = SZ[i + 1].v;

	SZ[3].v = gte_clamp(ctx, val, 0x0000, 0xFFFF, GTE_FLAG_SZ3_OTZ_SAT);

#undef SZ
}

P_NONNULL P_NODISCARD static s64 gte_div(struct p_ctx *ctx)
{
#define H   (ctx->cpu.cop2.ccr.h)
#define SZ3 (ctx->cpu.cop2.cpr.sz[3].v)

	s64 n;

	if (H < (SZ3 * 2)) {
		int z  = __builtin_clz(SZ3) - 16;
		n      = H << z;
		uint d = SZ3 << z;
		uint u = unr[(d - 0x7FC0) >> 7] + 0x101;
		d      = ((0x2000080 - (d * u)) >> 8);
		d      = ((0x0000080 + (d * u)) >> 8);
		n      = min(0x1FFFF, (((n * d) + 0x8000) >> 16));
	} else {
		flag_set(ctx, GTE_FLAG_DIV_OVF);
		n = 0x1FFFF;
	}
	return n;

#undef H
#undef SZ3
}

P_NONNULL static void rtp(struct p_ctx *ctx, const s16 *vec, bool dq)
{
#define TR    (ctx->cpu.cop2.ccr.tr)
#define RT    (ctx->cpu.cop2.ccr.r)
#define IR(n) (ctx->cpu.cop2.cpr.ir[(n)].v)
#define MAC   (ctx->cpu.cop2.cpr.mac)
#define MAC3  (MAC[3])
#define IR0   (IR(0))
#define IR1   (IR(1))
#define IR2   (IR(2))
#define IR3   (IR(3))
#define OFX   (ctx->cpu.cop2.ccr.ofx)
#define OFY   (ctx->cpu.cop2.ccr.ofy)
#define DQA   (ctx->cpu.cop2.ccr.dqa)
#define DQB   (ctx->cpu.cop2.ccr.dqb)

	const uint sf = shift_frac(ctx->cpu.instr);
	const bool lm = ir123_lm(ctx->cpu.instr);

	s64 sum;

	for (size_t i = 1; i < ARRAY_SIZE(MAC); ++i) {
		sum = 0;
		sum = mac123_add(ctx, i, sum, TR[i - 1] << 12);
		sum = mac123_add(ctx, i, sum, RT[i - 1][0] * vec[0]);
		sum = mac123_add(ctx, i, sum, RT[i - 1][1] * vec[1]);
		sum = mac123_add(ctx, i, sum, RT[i - 1][2] * vec[2]);

		// The last iteration of this loop will give us the 44-bit MAC3;
		// we'll need it to handle the special IR3 case
		MAC[i] = sum >> sf;
	}

	sum >>= 12;

	for (size_t i = 1; i < ARRAY_SIZE(MAC) - 1; ++i)
		IR(i) = ir123_sat(ctx, i, MAC[i], lm);

	IR3 = ir123_sat(ctx, 3, sum, false);
	IR3 = clamp(MAC3, lm ? GTE_IR123_LM_MIN : GTE_IR123_MIN, GTE_IR123_MAX);

	sz_push(ctx, sum);

	s64 quot = gte_div(ctx);

	sum = mac0_add(ctx, (quot * IR1) + OFX);
	sx_push(ctx, sum >> 16);

	sum = mac0_add(ctx, (quot * IR2) + OFY);
	sy_push(ctx, sum >> 16);

	if (dq) {
		sum = mac0_add(ctx, (quot * DQA) + DQB);
		IR0 = ir0_sat(ctx, sum >> 12);
	}
	MAC[0] = sum;
}

P_NONNULL static void do_cop2_ctc(struct p_ctx *ctx, size_t rd, size_t rt)
{
	switch (rd) {
	case P_FLAG:
		ctx->cpu.cop2.ccr.flag = ctx->cpu.gpr[rt] & GTE_FLAG_MASK;
		update_flag(ctx);

		break;

	default:
		ctx->cpu.cop2.ccr.raw[rd] = ctx->cpu.gpr[rt];
		break;
	}
}

P_NONNULL static void do_cop2_instr(struct p_ctx *ctx, uint funct)
{
	switch (funct) {
	case RTPT:
		ctx->cpu.cop2.ccr.flag = 0;

		rtp(ctx, ctx->cpu.cop2.cpr.v[0], false);
		rtp(ctx, ctx->cpu.cop2.cpr.v[1], false);
		rtp(ctx, ctx->cpu.cop2.cpr.v[2], true);

		update_flag(ctx);
		return;

	default:
		illegal_instr(ctx);
		return;
	}
}

P_NONNULL static void dly_slot_process(struct p_ctx *ctx)
{
	ctx->cpu.gpr[ctx->cpu.ld_next.dst] = ctx->cpu.ld_next.val;

	if (ctx->cpu.ld_next.dst)
		LOG_TRACE(ctx, "load delay eviction: %zu <- 0x%08X",
			  ctx->cpu.ld_next.dst, ctx->cpu.ld_next.val);

	memset(&ctx->cpu.ld_next, 0, sizeof(ctx->cpu.ld_next));
	swap(&ctx->cpu.ld_pend, &ctx->cpu.ld_next);
}

P_NONNULL static void load_dly(struct p_ctx *ctx, size_t dst, u32 val)
{
	if (unlikely(!dst)) {
		LOG_DBG(ctx, "Load delay rejected - dest was $zero");
		return;
	}

	ctx->cpu.ld_pend.dst = dst;
	ctx->cpu.ld_pend.val = val;

	LOG_TRACE(ctx, "load delay pending: dst=%zu, val=0x%08X", dst, val);

	if (unlikely(ctx->cpu.ld_next.dst == dst))
		memset(&ctx->cpu.ld_next, 0, sizeof(ctx->cpu.ld_next));
}

P_NONNULL static void step(struct p_ctx *ctx)
{
#define gpr	(ctx->cpu.gpr)
#define pc	(ctx->cpu.pc)
#define npc	(ctx->cpu.npc)
#define hi	(ctx->cpu.hi)
#define lo	(ctx->cpu.lo)
#define instr	(ctx->cpu.instr)

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

	if (unlikely(ctx->cpu.dly_pc & 3))
		exc(ctx, EXC_ADEL);

	ctx->cpu.in_bd	    = ctx->cpu.next_in_bd;
	ctx->cpu.next_in_bd = false;

	pc    = ctx->cpu.dly_pc;
	instr = load32(ctx, pc);

	ctx->cpu.dly_pc = npc;
	npc		= ctx->cpu.dly_pc + sizeof(instr);

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
			gpr_set(ctx, rt, ctx->cpu.cop0[rd]);
			break;

		case MTC:
			ctx->cpu.cop0[rd] = gpr[rt];
			break;

		default:
			do_cop0_instr(ctx, funct);
			break;
		}
		break;

	case GRP_COP2:
		switch (rs) {
		case MFC:
			gpr_set(ctx, rt, ctx->cpu.cop2.cpr.raw[rd]);
			break;

		case CFC:
			gpr[rt] = ctx->cpu.cop2.ccr.raw[rd];
			break;

		case MTC:
			ctx->cpu.cop2.cpr.raw[rd] = gpr[rt];
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

		u32 val = (ctx->cpu.ld_next.dst == rt) ? ctx->cpu.ld_next.val :
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

		u32 val = (ctx->cpu.ld_next.dst == rt) ? ctx->cpu.ld_next.val :
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

void p_cpu_irq_mux_set(struct p_ctx *ctx, bool set)
{
	if (set) {
		ctx->cpu.cop0[P_SR] |= (1 << 10);
		LOG_DBG(ctx, "irq mux line asserted");
	} else {
		ctx->cpu.cop0[P_SR] &= ~(1 << 10);
		LOG_DBG(ctx, "irq mux line not asserted");
	}
}

void p_cpu_pc_set(struct p_ctx *ctx, u32 pc)
{
	ctx->cpu.dly_pc = pc;
	ctx->cpu.pc	= pc;
	ctx->cpu.npc	= pc + sizeof(ctx->cpu.instr);
}

void p_cpu_gpr_set(struct p_ctx *ctx, enum p_cpu_gpr gpr, u32 val)
{
	assert(gpr < P_GPR_COUNT);
	ctx->cpu.gpr[gpr] = val;
}

void p_cpu_rst(struct p_ctx *ctx)
{
	memset(ctx->cpu.gpr, 0, sizeof(ctx->cpu.gpr));
	p_cpu_pc_set(ctx, RST_VECTOR);

	memset(&ctx->cpu.ld_pend, 0, sizeof(ctx->cpu.ld_pend));
	memset(&ctx->cpu.ld_next, 0, sizeof(ctx->cpu.ld_next));

	LOG_INFO(ctx, "reset");
}

void p_cpu_run(struct p_ctx *ctx, u64 cycles)
{
	cycles += ctx->sched.ts_now;

	while (ctx->sched.ts_now < cycles) {
		p_bios_trace_begin(ctx);
		step(ctx);
		p_bios_trace_end(ctx);
	}
}
