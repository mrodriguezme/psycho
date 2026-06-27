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

#include "bus.h"
#include "cpu.h"
#include "cpu_defs.h"
#include "disasm.h"
#include "log.h"
#include "util.h"
#include "sched.h"

LOG_MOD(P_LOG_CPU);

__attribute__((nonnull)) static void illegal_instr(struct p_ctx *const ctx)
{
	LOG_ERR(ctx, "illegal instruction trapped (pc=0x%08X, instr=0x%08X)",
		ctx->cpu.pc, ctx->cpu.instr);

	ctx->cfg.cpu.illegal_instr(ctx, ctx->cpu.instr);
}

P_NODISCARD __attribute__((nonnull)) static u32
load_word(struct p_ctx *const ctx, u32 vaddr)
{
	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return p_load_word(ctx, vaddr);
}

P_NODISCARD __attribute__((nonnull)) static u16
load_halfword(struct p_ctx *const ctx, u32 vaddr)
{
	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return p_load_halfword(ctx, vaddr);
}

P_NODISCARD __attribute__((nonnull)) static u8
load_byte(struct p_ctx *const ctx, u32 vaddr)
{
	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	return p_load_byte(ctx, vaddr);
}

__attribute__((nonnull)) static void store_word(struct p_ctx *const ctx,
						u32 vaddr, const u32 word)
{
	p_sched_adv_ts(ctx, 1);

	if (ctx->cpu.cop0[P_SR] & SR_ISC)
		return;

	vaddr = vaddr_to_paddr(vaddr);
	p_store_word(ctx, vaddr, word);
}

__attribute__((nonnull)) static void
store_halfword(struct p_ctx *const ctx, u32 vaddr, const u16 halfword)
{
	if (ctx->cpu.cop0[P_SR] & SR_ISC)
		return;

	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	p_store_halfword(ctx, vaddr, halfword);
}

__attribute__((nonnull)) static void store_byte(struct p_ctx *const ctx,
						u32 vaddr, const u8 byte)
{
	if (ctx->cpu.cop0[P_SR] & SR_ISC)
		return;

	p_sched_adv_ts(ctx, 1);

	vaddr = vaddr_to_paddr(vaddr);
	p_store_byte(ctx, vaddr, byte);
}

__attribute__((nonnull)) static void disasm_capture(struct p_ctx *const ctx)
{
	if (ctx->cfg.disasm.tracing) {
		p_disasm_trace_begin(ctx, ctx->cpu.pc);
		return;
	}

	if (MOD_LOG_LVL_ACTIVE(ctx, P_LOG_TRACE)) {
		p_disasm_instr(ctx, ctx->cpu.pc, NULL);
		LOG_TRACE_UNCHECKED(ctx, "[disasm] 0x%08X: %s",
				    ctx->disasm.res.pc,
				    ctx->disasm.res.str.ptr);
	}
}

__attribute__((nonnull)) static void disasm_emit(struct p_ctx *const ctx)
{
	if (!ctx->cfg.disasm.tracing)
		return;

	p_disasm_trace_end(ctx);
	LOG_TRACE(ctx, "[disasm] 0x%08X: %s", ctx->disasm.res.pc,
		  ctx->disasm.res.str.ptr);
}

__attribute__((nonnull)) static void gpr_set(struct p_ctx *const ctx,
					     const size_t reg, const u32 val)
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

__attribute__((nonnull)) static void branch(struct p_ctx *const ctx,
					    const u32 addr)
{
	ctx->cpu.next_in_bd = true;
	ctx->cpu.npc = addr;
}

__attribute__((nonnull)) static void branch_if(struct p_ctx *const ctx,
					       const bool cond)
{
	ctx->cpu.next_in_bd = true;

	if (cond) {
		const u32 pc = unlikely(ctx->cpu.in_bd) ?
				       ctx->cpu.dly_pc - sizeof(u32) :
				       ctx->cpu.pc;

		ctx->cpu.npc = branch_addr(pc, ctx->cpu.instr);
	}
}

__attribute__((nonnull)) static void exc(struct p_ctx *const ctx,
					 const enum cpu_exc exc)
{
#define SR (ctx->cpu.cop0[P_SR])
#define CAUSE (ctx->cpu.cop0[P_CAUSE])
#define EPC (ctx->cpu.cop0[P_EPC])

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

__attribute__((nonnull)) static void
do_div(struct p_ctx *const ctx, const s32 dividend, const s32 divisor)
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

__attribute__((nonnull)) static void
do_divu(struct p_ctx *const ctx, const u32 dividend, const u32 divisor)
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

__attribute__((nonnull)) static void
do_add(struct p_ctx *const ctx, const size_t dst, const u32 a0, const u32 a1)
{
	int sum;

	if (unlikely(__builtin_sadd_overflow(a0, a1, &sum)))
		exc(ctx, EXC_OV);
	else
		gpr_set(ctx, dst, sum);
}

__attribute__((nonnull)) static void do_sub(struct p_ctx *const ctx,
					    const size_t dst, const u32 minuend,
					    const u32 subtrahend)
{
	int diff;

	if (unlikely(__builtin_ssub_overflow(minuend, subtrahend, &diff)))
		exc(ctx, EXC_OV);
	else
		gpr_set(ctx, dst, diff);
}

__attribute__((nonnull)) static void do_cop0_instr(struct p_ctx *const ctx,
						   const uint funct)
{
#define SR (ctx->cpu.cop0[P_SR])

	if (unlikely(funct != RFE))
		illegal_instr(ctx);
	else
		SR = (SR & ~0x0F) | ((SR >> 2) & 0x0F);

#undef SR
}

__attribute__((nonnull)) static void dly_slot_process(struct p_ctx *const ctx)
{
	ctx->cpu.gpr[ctx->cpu.ld_next.dst] = ctx->cpu.ld_next.val;
	memset(&ctx->cpu.ld_next, 0, sizeof(ctx->cpu.ld_next));
	swap(&ctx->cpu.ld_pend, &ctx->cpu.ld_next);
}

__attribute__((nonnull)) static void load_dly(struct p_ctx *const ctx,
					      const size_t dst, const u32 val)
{
	if (unlikely(!dst)) {
		LOG_WARN(ctx, "Load delay rejected - dest was $zero");
		return;
	}

	ctx->cpu.ld_pend.dst = dst;
	ctx->cpu.ld_pend.val = val;

	if (unlikely(ctx->cpu.ld_next.dst == dst))
		memset(&ctx->cpu.ld_next, 0, sizeof(ctx->cpu.ld_next));
}

void p_cpu_pc_set(struct p_ctx *const ctx, const u32 pc)
{
	ctx->cpu.dly_pc = pc;
	ctx->cpu.pc = pc;
	ctx->cpu.npc = pc + sizeof(ctx->cpu.instr);
}

void p_cpu_gpr_set(struct p_ctx *const ctx, const enum p_cpu_gpr gpr,
		   const u32 val)
{
	assert(gpr < P_GPR_COUNT);
	ctx->cpu.gpr[gpr] = val;
}

void p_cpu_run(struct p_ctx *const ctx, u64 cycles)
{
	cycles += ctx->sched.ts_now;

	while (ctx->sched.ts_now < cycles)
		p_cpu_step(ctx);
}

void p_cpu_rst(struct p_ctx *const ctx)
{
	memset(ctx->cpu.gpr, 0, sizeof(ctx->cpu.gpr));
	p_cpu_pc_set(ctx, RST_VECTOR);

	memset(&ctx->cpu.ld_pend, 0, sizeof(ctx->cpu.ld_pend));
	memset(&ctx->cpu.ld_next, 0, sizeof(ctx->cpu.ld_next));

	LOG_INFO(ctx, "reset");
}

void p_cpu_step(struct p_ctx *const ctx)
{
#define gpr (ctx->cpu.gpr)
#define pc (ctx->cpu.pc)
#define npc (ctx->cpu.npc)
#define hi (ctx->cpu.hi)
#define lo (ctx->cpu.lo)
#define instr (ctx->cpu.instr)

#define op (instr_op(instr))
#define rt (instr_rt(instr))
#define rs (instr_rs(instr))
#define rd (instr_rd(instr))
#define shamt (instr_shamt(instr))
#define funct (instr_funct(instr))
#define base (rs)
#define zextimm (zext_16_32(instr_imm(instr)))
#define sextimm (sext_16_32(instr_imm(instr)))
#define offset (sextimm)

	if (unlikely(ctx->cpu.dly_pc & 3))
		exc(ctx, EXC_ADEL);

	ctx->cpu.in_bd = ctx->cpu.next_in_bd;
	ctx->cpu.next_in_bd = false;

	pc = ctx->cpu.dly_pc;
	instr = load_word(ctx, pc);

	ctx->cpu.dly_pc = npc;
	npc = ctx->cpu.dly_pc + sizeof(instr);

	disasm_capture(ctx);

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
			const u32 jmp_addr = gpr[rs];

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
			const u64 x = sext_32_64(gpr[rs]) * sext_32_64(gpr[rt]);

			lo = x & UINT32_MAX;
			hi = x >> 32;

			break;
		}

		case MULTU: {
			const u64 x = zext_32_64(gpr[rs]) * zext_32_64(gpr[rt]);

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
		const bool link = (rt & 0x1E) == 0x10;
		const bool branch = (s32)(gpr[rs] ^ (rt << 31)) < 0;

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

	case LB:
		load_dly(ctx, rt,
			 sext_8_32(load_byte(ctx, gpr[base] + offset)));
		break;

	case LH: {
		const u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 1)) {
			exc(ctx, EXC_ADEL);
			break;
		}
		load_dly(ctx, rt, sext_16_32(load_halfword(ctx, vaddr)));
		break;
	}

	case LWL: {
		const u32 vaddr = gpr[base] + offset;
		const u32 aligned_vaddr = vaddr & ~3;

		const u32 word = load_word(ctx, aligned_vaddr);

		const uint shift = (vaddr & 3) * 8;
		const uint mask = 0x00FFFFFF >> shift;

		u32 val = (ctx->cpu.ld_next.dst == rt) ? ctx->cpu.ld_next.val :
							 gpr[rt];

		val = (val & mask) | (word << (24 - shift));
		load_dly(ctx, rt, val);

		break;
	}

	case LW: {
		const u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 0x3)) {
			exc(ctx, EXC_ADEL);
			break;
		}
		load_dly(ctx, rt, load_word(ctx, vaddr));
		break;
	}

	case LBU:
		load_dly(ctx, rt,
			 zext_8_32(load_byte(ctx, gpr[base] + offset)));
		break;

	case LHU: {
		const u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 1)) {
			exc(ctx, EXC_ADEL);
			break;
		}

		load_dly(ctx, rt, zext_16_32(load_halfword(ctx, vaddr)));
		break;
	}

	case LWR: {
		const u32 vaddr = gpr[base] + offset;
		const u32 aligned_vaddr = vaddr & ~3;

		const u32 word = load_word(ctx, aligned_vaddr);

		const uint shift = (vaddr & 3) * 8;
		const uint mask = 0xFFFFFF00 << (24 - shift);

		u32 val = (ctx->cpu.ld_next.dst == rt) ? ctx->cpu.ld_next.val :
							 gpr[rt];

		val = (val & mask) | (word >> shift);

		load_dly(ctx, rt, val);
		break;
	}

	case SB:
		store_byte(ctx, gpr[base] + offset, gpr[rt] & UINT8_MAX);
		break;

	case SH: {
		const u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 1)) {
			exc(ctx, EXC_ADES);
			break;
		}

		store_halfword(ctx, vaddr, gpr[rt] & UINT16_MAX);
		break;
	}

	case SWL: {
		const u32 vaddr = gpr[base] + offset;
		const u32 aligned_vaddr = vaddr & ~3;

		const uint shift = (vaddr & 3) * 8;
		const uint mask = 0xFFFFFF00 << shift;

		u32 word = load_word(ctx, aligned_vaddr);
		word = (word & mask) | (gpr[rt] >> (24 - shift));
		store_word(ctx, aligned_vaddr, word);

		break;
	}

	case SW: {
		const u32 vaddr = gpr[base] + offset;

		if (unlikely(vaddr & 3)) {
			exc(ctx, EXC_ADES);
			break;
		}

		store_word(ctx, vaddr, gpr[rt]);
		break;
	}

	case SWR: {
		const u32 vaddr = gpr[base] + offset;
		const u32 aligned_vaddr = vaddr & ~3;

		const uint shift = (vaddr & 3) * 8;
		const uint mask = 0x00FFFFFF >> (24 - shift);

		u32 word = load_word(ctx, aligned_vaddr);
		word = (word & mask) | (gpr[rt] << shift);
		store_word(ctx, aligned_vaddr, word);

		break;
	}

	default:
		illegal_instr(ctx);
		break;
	}

	// Better than a branch - ensure that zero is indeed always zero.
	gpr[P_ZERO] = 0;

	disasm_emit(ctx);
}
