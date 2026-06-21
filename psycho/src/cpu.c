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
	vaddr = vaddr_to_paddr(vaddr);
	return p_load_word(ctx, vaddr);
}

P_NODISCARD __attribute__((nonnull)) static u16
load_halfword(struct p_ctx *const ctx, u32 vaddr)
{
	vaddr = vaddr_to_paddr(vaddr);
	return p_load_halfword(ctx, vaddr);
}

P_NODISCARD __attribute__((nonnull)) static u8
load_byte(struct p_ctx *const ctx, u32 vaddr)
{
	vaddr = vaddr_to_paddr(vaddr);
	return p_load_byte(ctx, vaddr);
}

__attribute__((nonnull)) static void store_word(struct p_ctx *const ctx,
						u32 vaddr, const u32 word)
{
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

	vaddr = vaddr_to_paddr(vaddr);
	p_store_halfword(ctx, vaddr, halfword);
}

__attribute__((nonnull)) static void store_byte(struct p_ctx *const ctx,
						u32 vaddr, const u8 byte)
{
	if (ctx->cpu.cop0[P_SR] & SR_ISC)
		return;

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

__attribute__((nonnull)) static void branch_if(struct p_ctx *const ctx,
					       const bool cond)
{
	if (cond)
		ctx->cpu.npc = branch_addr(ctx->cpu.pc, ctx->cpu.instr);
}

__attribute__((nonnull)) static void exc(struct p_ctx *const ctx,
					 const enum cpu_exc exc)
{
#define EPC (ctx->cpu.cop0[P_EPC])
#define CAUSE (ctx->cpu.cop0[P_CAUSE])
#define SR (ctx->cpu.cop0[P_SR])

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

#undef EPC
#undef CAUSE
}

__attribute__((nonnull)) static void
do_div(struct p_ctx *const ctx, const s32 dividend, const s32 divisor)
{
#define lo (ctx->cpu.lo)
#define hi (ctx->cpu.hi)

	if (!divisor) {
		// That is, if the dividend is negative, the
		// quotient is 1 (0x00000001), and if the
		// dividend is positive or zero, the quotient is
		// -1 (0xFFFFFFFF).
		lo = (dividend < 0) ? 1 : UINT32_MAX;

		// In both cases the remainder equals the
		// dividend.
		hi = dividend;
	} else if ((dividend == INT32_MIN) && (divisor == -1)) {
		lo = dividend;
		hi = 0;
	} else {
		lo = dividend / divisor;
		hi = dividend % divisor;
	}

#undef lo
#undef hi
}

__attribute__((nonnull)) static void
do_divu(struct p_ctx *const ctx, const u32 dividend, const u32 divisor)
{
#define lo (ctx->cpu.lo)
#define hi (ctx->cpu.hi)

	if (!divisor) {
		// In the case of unsigned division, the dividend can't be
		// negative and thus the quotient is always -1 (0xFFFFFFFF) and
		// the remainder equals the dividend.
		lo = UINT32_MAX;
		hi = dividend;
	} else {
		lo = dividend / divisor;
		hi = dividend % divisor;
	}

#undef lo
#undef hi
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

void p_cpu_rst(struct p_ctx *const ctx)
{
	memset(ctx->cpu.gpr, 0, sizeof(ctx->cpu.gpr));
	p_cpu_pc_set(ctx, RST_VECTOR);

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

	if (ctx->cpu.dly_pc & 3)
		exc(ctx, EXC_ADEL);

	pc = ctx->cpu.dly_pc;
	instr = load_word(ctx, pc);

	ctx->cpu.dly_pc = npc;
	npc = ctx->cpu.dly_pc + sizeof(instr);

	disasm_capture(ctx);

	switch (op) {
	case GRP_SPECIAL:
		switch (funct) {
		case SLL:
			gpr[rd] = gpr[rt] << shamt;
			break;

		case SRL:
			gpr[rd] = gpr[rt] >> shamt;
			break;

		case SRA:
			gpr[rd] = (s32)gpr[rt] >> shamt;
			break;

		case SLLV:
			gpr[rd] = gpr[rt] << (gpr[rs] & 0x1F);
			break;

		case SRLV:
			gpr[rd] = gpr[rt] >> (gpr[rs] & 0x1F);
			break;

		case SRAV:
			gpr[rd] = (s32)gpr[rt] >> (gpr[rs] & 0x1F);
			break;

		case JR:
			npc = gpr[rs];
			break;

		case JALR: {
			const u32 jmp_addr = gpr[rs];

			gpr[rd] = pc + (sizeof(instr) * 2);
			npc = jmp_addr;

			break;
		}

		case SYSCALL:
			exc(ctx, EXC_SYSCALL);
			break;

		case BREAK:
			exc(ctx, EXC_BP);
			break;

		case MFHI:
			gpr[rd] = hi;
			break;

		case MTHI:
			hi = gpr[rs];
			break;

		case MFLO:
			gpr[rd] = lo;
			break;

		case MTLO:
			lo = gpr[rs];
			break;

		case MULT: {
			const u64 prod =
				sext_32_64(gpr[rs]) * sext_32_64(gpr[rt]);

			lo = prod & UINT32_MAX;
			hi = prod >> 32;

			break;
		}

		case MULTU: {
			const u64 prod =
				zext_32_64(gpr[rs]) * zext_32_64(gpr[rt]);

			lo = prod & UINT32_MAX;
			hi = prod >> 32;

			break;
		}

		case DIV:
			do_div(ctx, gpr[rs], gpr[rt]);
			break;

		case DIVU:
			do_divu(ctx, gpr[rs], gpr[rt]);
			break;

		case ADD: {
			int sum;

			if (__builtin_sadd_overflow(gpr[rs], gpr[rt], &sum)) {
				exc(ctx, EXC_OV);
				break;
			}

			gpr[rd] = sum;
			break;
		}

		case ADDU:
			gpr[rd] = gpr[rs] + gpr[rt];
			break;

		case SUB: {
			int diff;

			if (__builtin_ssub_overflow(gpr[rs], gpr[rt], &diff)) {
				exc(ctx, EXC_OV);
				break;
			}

			gpr[rd] = diff;
			break;
		}

		case SUBU:
			gpr[rd] = gpr[rs] - gpr[rt];
			break;

		case AND:
			gpr[rd] = gpr[rs] & gpr[rt];
			break;

		case OR:
			gpr[rd] = gpr[rs] | gpr[rt];
			break;

		case XOR:
			gpr[rd] = gpr[rs] ^ gpr[rt];
			break;

		case NOR:
			gpr[rd] = ~(gpr[rs] | gpr[rt]);
			break;

		case SLT:
			gpr[rd] = (s32)gpr[rs] < (s32)gpr[rt];
			break;

		case SLTU:
			gpr[rd] = gpr[rs] < gpr[rt];
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
			gpr[P_RA] = pc + (sizeof(instr) * 2);

		branch_if(ctx, branch);
		break;
	}

	case J:
		npc = jmp_addr(pc, instr);
		break;

	case JAL:
		gpr[P_RA] = pc + (sizeof(instr) * 2);
		npc = jmp_addr(pc, instr);

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

		if (__builtin_sadd_overflow(gpr[rs], sextimm, &sum)) {
			exc(ctx, EXC_OV);
			break;
		}

		gpr[rt] = sum;
		break;
	}

	case ADDIU:
		gpr[rt] = gpr[rs] + sextimm;
		break;

	case SLTI:
		gpr[rt] = (s32)gpr[rs] < (s32)sextimm;
		break;

	case SLTIU:
		gpr[rt] = gpr[rs] < sextimm;
		break;

	case ANDI:
		gpr[rt] = zextimm & gpr[rs];
		break;

	case ORI:
		gpr[rt] = zextimm | gpr[rs];
		break;

	case XORI:
		gpr[rt] = zextimm ^ gpr[rs];
		break;

	case LUI:
		gpr[rt] = zextimm << 16;
		break;

	case GRP_COP0:
		switch (rs) {
		case MFC:
			gpr[rt] = ctx->cpu.cop0[rd];
			break;

		case MTC:
			ctx->cpu.cop0[rd] = gpr[rt];
			break;

		default:
			switch (funct) {
			case RFE:
				SR = (SR & ~0x0F) | ((SR >> 2) & 0x0F);
				break;

			default:
				illegal_instr(ctx);
				break;
			}
		}
		break;

	case LB:
		gpr[rt] = sext_8_32(load_byte(ctx, gpr[base] + offset));
		break;

	case LH: {
		const u32 vaddr = gpr[base] + offset;

		if (vaddr & 1) {
			exc(ctx, EXC_ADEL);
			break;
		}
		gpr[rt] = sext_16_32(load_halfword(ctx, vaddr));
		break;
	}

	case LWL: {
		const u32 vaddr = gpr[base] + offset;
		const u32 aligned_vaddr = vaddr & ~3;

		const u32 word = load_word(ctx, aligned_vaddr);

		const uint shift = (vaddr & 3) * 8;
		const uint mask = 0x00FFFFFF >> shift;

		gpr[rt] = (gpr[rt] & mask) | (word << (24 - shift));
		break;
	}

	case LW: {
		const u32 vaddr = gpr[base] + offset;

		if (vaddr & 0x3) {
			exc(ctx, EXC_ADEL);
			break;
		}
		gpr[rt] = load_word(ctx, vaddr);
		break;
	}

	case LBU:
		gpr[rt] = zext_8_32(load_byte(ctx, gpr[base] + offset));
		break;

	case LHU: {
		const u32 vaddr = gpr[base] + offset;

		if (vaddr & 1) {
			exc(ctx, EXC_ADEL);
			break;
		}

		gpr[rt] = zext_16_32(load_halfword(ctx, vaddr));
		break;
	}

	case LWR: {
		const u32 vaddr = gpr[base] + offset;
		const u32 aligned_vaddr = vaddr & ~3;

		const u32 word = load_word(ctx, aligned_vaddr);

		const uint shift = (vaddr & 3) * 8;
		const uint mask = 0xFFFFFF00 << (24 - shift);

		gpr[rt] = (gpr[rt] & mask) | (word >> shift);
		break;
	}

	case SB:
		store_byte(ctx, gpr[base] + offset, gpr[rt] & UINT8_MAX);
		break;

	case SH: {
		const u32 vaddr = gpr[base] + offset;

		if (vaddr & 1) {
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

		if (vaddr & 3) {
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
