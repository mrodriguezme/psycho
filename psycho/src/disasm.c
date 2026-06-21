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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "psycho/compiler.h"
#include "psycho/ctx.h"

#include "bus.h"
#include "cpu_defs.h"
#include "disasm.h"
#include "str.h"

enum {
	TRACE_NUM_SPACES = 40,
};

static const char *const gpr[P_GPR_COUNT] = {
	// clang-format off

	[P_ZERO]	= "$zero",
	[P_AT]		= "$at",
	[P_V0]		= "$v0",
	[P_V1]		= "$v1",
	[P_A0]		= "$a0",
	[P_A1]		= "$a1",
	[P_A2]		= "$a2",
	[P_A3]		= "$a3",
	[P_T0]		= "$t0",
	[P_T1]		= "$t1",
	[P_T2]		= "$t2",
	[P_T3]		= "$t3",
	[P_T4]		= "$t4",
	[P_T5]		= "$t5",
	[P_T6]		= "$t6",
	[P_T7]		= "$t7",
	[P_S0]		= "$s0",
	[P_S1]		= "$s1",
	[P_S2]		= "$s2",
	[P_S3]		= "$s3",
	[P_S4]		= "$s4",
	[P_S5]		= "$s5",
	[P_S6]		= "$s6",
	[P_S7]		= "$s7",
	[P_T8]		= "$t8",
	[P_T9]		= "$t9",
	[P_K0]		= "$k0",
	[P_K1]		= "$k1",
	[P_GP]		= "$gp",
	[P_SP]		= "$sp",
	[P_FP]		= "$fp",
	[P_RA]		= "$ra"

	// clang-format on
};

static const char *const cop0[P_COP0_COUNT] = {
	// clang-format off

	[0]		= "C0_UNUSED0",
	[1]		= "C0_UNUSED1",
	[2]		= "C0_UNUSED2",
	[P_BPC]		= "C0_BPC",
	[4]		= "C0_UNUSED4",
	[P_BDA]		= "C0_BDA",
	[P_TAR]		= "C0_TAR",
	[P_DCIC]	= "C0_DCIC",
	[P_BADVADDR]	= "C0_BADVADDR",
	[P_BDAM]	= "C0_BDAM",
	[10]		= "C0_UNUSED10",
	[P_BPCM]	= "C0_BPCM",
	[P_SR]		= "C0_SR",
	[P_CAUSE]	= "C0_CAUSE",
	[P_EPC]		= "C0_EPC",
	[P_PRID]	= "C0_PRID",
	[16]		= "C0_UNUSED16",
	[17]		= "C0_UNUSED17",
	[18]		= "C0_UNUSED18",
	[19]		= "C0_UNUSED19",
	[20]		= "C0_UNUSED20",
	[21]		= "C0_UNUSED21",
	[22]		= "C0_UNUSED22",
	[23]		= "C0_UNUSED23",
	[24]		= "C0_UNUSED24",
	[25]		= "C0_UNUSED25",
	[26]		= "C0_UNUSED26",
	[27]		= "C0_UNUSED27",
	[28]		= "C0_UNUSED28",
	[29]		= "C0_UNUSED29",
	[30]		= "C0_UNUSED30",
	[31]		= "C0_UNUSED31"

	// clang-format on
};

P_NODISCARD __attribute__((nonnull)) static u32
instr_get(struct p_ctx *const ctx, u32 pc)
{
	pc = vaddr_to_paddr(pc);
	return p_load_word(ctx, pc);
}

static void trace_add(struct p_disasm_traces *const traces,
		      const enum p_disasm_trace trace)
{
	if (traces) {
		assert(traces->count < ARRAY_SIZE(traces->data));
		traces->data[traces->count++] = trace;
	}
}

P_NODISCARD const char *p_gpr_get(const enum p_cpu_gpr reg)
{
	assert(reg < P_GPR_COUNT);
	return gpr[reg];
}

P_NODISCARD const char *p_cop0_get(const enum p_cpu_cop0 reg)
{
	assert(reg < P_COP0_COUNT);
	return cop0[reg];
}

void p_disasm_instr(struct p_ctx *const ctx, const u32 pc,
		    struct p_disasm_traces *traces)
{
	p_str_init_fixed(&ctx->disasm.res.str, ctx->disasm.res.str_buf,
			 sizeof(ctx->disasm.res.str_buf));

	const u32 instr = instr_get(ctx, pc);

	ctx->disasm.res.instr = instr;
	ctx->disasm.res.pc = pc;

#define fmt(args...)                                                  \
	({                                                            \
		bool truncated;                                       \
		p_str_append(&ctx->disasm.res.str, &truncated, args); \
		assert(!truncated);                                   \
	})

#define op (instr_op(instr))
#define rt (instr_rt(instr))
#define rs (instr_rs(instr))
#define rd (instr_rd(instr))
#define shamt (instr_shamt(instr))
#define funct (instr_funct(instr))
#define imm (instr_imm(instr))
#define simm ((s16)imm)
#define offset (simm)
#define base (rs)
#define branch_addr (branch_addr(pc, instr))

	switch (op) {
	case GRP_SPECIAL:
		switch (funct) {
		case SLL:
			fmt("sll %s, %s, 0x%X", gpr[rd], gpr[rt], shamt);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SRL:
			fmt("srl %s, %s, %u", gpr[rd], gpr[rt], shamt);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SRA:
			fmt("sra %s, %s, %u", gpr[rd], gpr[rt], shamt);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SLLV:
			fmt("sllv %s, %s, %s", gpr[rd], gpr[rt], gpr[rs]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SRLV:
			fmt("srlv %s, %s, %s", gpr[rd], gpr[rt], gpr[rs]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SRAV:
			fmt("srav %s, %s, %s", gpr[rd], gpr[rt], gpr[rs]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case JR:
			fmt("jr %s", gpr[rs]);
			return;

		case JALR:
			fmt("jalr %s, %s", gpr[rd], gpr[rs]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case MFHI:
			fmt("mfhi %s", gpr[rd]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case MTHI:
			fmt("mthi %s", gpr[rs]);
			trace_add(traces, P_DISASM_TRACE_CPU_HI);

			return;

		case MFLO:
			fmt("mflo %s", gpr[rd]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case MTLO:
			fmt("mtlo %s", gpr[rs]);
			trace_add(traces, P_DISASM_TRACE_CPU_LO);

			return;

		case MULT:
			fmt("mult %s, %s", gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_CPU_LO);
			trace_add(traces, P_DISASM_TRACE_CPU_HI);

			break;

		case MULTU:
			fmt("multu %s, %s", gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_CPU_LO);
			trace_add(traces, P_DISASM_TRACE_CPU_HI);

			break;

		case DIV:
			fmt("div %s, %s", gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_CPU_LO);
			trace_add(traces, P_DISASM_TRACE_CPU_HI);

			return;

		case DIVU:
			fmt("divu %s, %s", gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_CPU_LO);
			trace_add(traces, P_DISASM_TRACE_CPU_HI);

			return;

		case ADD:
			fmt("add %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);

			trace_add(traces, P_DISASM_TRACE_GPR_RD);
			return;

		case ADDU:
			fmt("addu %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SUB:
			fmt("sub %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SUBU:
			fmt("subu %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case AND:
			fmt("and %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case OR:
			fmt("or %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case XOR:
			fmt("xor %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case NOR:
			fmt("nor %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SLT:
			fmt("slt %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_GPR_RD);

			return;

		case SLTU:
			fmt("sltu %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);

			trace_add(traces, P_DISASM_TRACE_GPR_RD);
			return;

		default:
			break;
		}
		break;

	case GRP_REGIMM:
		switch (rt) {
		case BLTZ:
			fmt("bltz %s, 0x%08X", gpr[rs], branch_addr);
			return;

		case BGEZ:
			fmt("bgez %s, 0x%08X", gpr[rs], branch_addr);
			return;

		case BLTZAL:
			fmt("bltzal %s, 0x%08X", gpr[rs], branch_addr);
			return;

		default:
			break;
		}
		break;

	case J:
		fmt("j 0x%08X", jmp_addr(pc, instr));
		return;

	case JAL:
		fmt("jal 0x%08X", jmp_addr(pc, instr));
		return;

	case BEQ:
		fmt("beq %s, %s, 0x%08X", gpr[rs], gpr[rt], branch_addr);
		return;

	case BNE:
		fmt("bne %s, %s, 0x%08X", gpr[rs], gpr[rt], branch_addr);
		return;

	case BLEZ:
		fmt("blez %s, 0x%08X", gpr[rs], branch_addr);
		return;

	case BGTZ:
		fmt("bgtz %s, 0x%08X", gpr[rs], branch_addr);
		return;

	case ADDI:
		fmt("addi %s, %s, %d", gpr[rt], gpr[rs], simm);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case ADDIU:
		fmt("addiu %s, %s, %d", gpr[rt], gpr[rs], simm);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case SLTI:
		fmt("slti %s, %s, %d", gpr[rt], gpr[rs], simm);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case SLTIU:
		fmt("sltiu %s, %s, %d", gpr[rt], gpr[rs], simm);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case ANDI:
		fmt("andi %s, %s, 0x%04X", gpr[rt], gpr[rs], imm);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case ORI:
		fmt("ori %s, %s, 0x%04X", gpr[rt], gpr[rs], imm);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case XORI:
		fmt("xori %s, %s, 0x%04X", gpr[rt], gpr[rs], imm);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case LUI:
		fmt("lui %s, 0x%04X", gpr[rt], imm);

		trace_add(traces, P_DISASM_TRACE_GPR_RT);
		return;

	case GRP_COP0:
		switch (rs) {
		case MFC:
			fmt("mfc0 %s, %s", gpr[rt], cop0[rd]);
			trace_add(traces, P_DISASM_TRACE_GPR_RT);

			return;

		case MTC:
			fmt("mtc0 %s, %s", cop0[rd], gpr[rt]);
			return;

		default:
			switch (funct) {
			case RFE:
				fmt("rfe");
				return;

			default:
				break;
			}
			break;
		}
		break;

	case LB:
		fmt("lb %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case LH:
		fmt("lh %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case LWL:
		fmt("lwl %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case LW:
		fmt("lw %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case LBU:
		fmt("lbu %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case LHU:
		fmt("lhu %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case LWR:
		fmt("lwr %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);

		return;

	case SB:
		fmt("sb %s, %d(%s)", gpr[rt], offset, gpr[base]);
		return;

	case SH:
		fmt("sh %s, %d(%s)", gpr[rt], offset, gpr[base]);
		return;

	case SWL:
		fmt("swl %s, %d(%s)", gpr[rt], offset, gpr[base]);
		return;

	case SW:
		fmt("sw %s, %d(%s)", gpr[rt], offset, gpr[base]);
		return;

	case SWR:
		fmt("swr %s, %d(%s)", gpr[rt], offset, gpr[base]);
		return;

	default:
		break;
	}

	fmt("illegal 0x%08X", instr);

#undef fmt
#undef op
#undef rt
#undef rs
#undef rd
#undef shamt
#undef funct
#undef imm
#undef offset
#undef base
#undef branch_addr
}

void p_disasm_trace_begin(struct p_ctx *const ctx, const u32 pc)
{
	memset(&ctx->disasm.traces, 0, sizeof(ctx->disasm.traces));
	p_disasm_instr(ctx, pc, &ctx->disasm.traces);

	if (!ctx->disasm.traces.count)
		return;

	bool truncated;

	p_str_pad(&ctx->disasm.res.str, ' ', TRACE_NUM_SPACES, &truncated);
	assert(!truncated);

	p_str_append(&ctx->disasm.res.str, &truncated, "; ");
	assert(!truncated);
}

void p_disasm_trace_end(struct p_ctx *const ctx)
{
#define fmt(args...)                                                  \
	({                                                            \
		bool truncated;                                       \
		p_str_append(&ctx->disasm.res.str, &truncated, args); \
		assert(!truncated);                                   \
	})

#define rt (instr_rt(ctx->disasm.res.instr))
#define rd (instr_rd(ctx->disasm.res.instr))

	for (size_t trace = 0; trace < ctx->disasm.traces.count; ++trace) {
		if (trace)
			fmt(", ");

		switch (ctx->disasm.traces.data[trace]) {
		case P_DISASM_TRACE_GPR_RT:
			fmt("%s=0x%08X", gpr[rt], ctx->cpu.gpr[rt]);
			break;

		case P_DISASM_TRACE_GPR_RD:
			fmt("%s=0x%08X", gpr[rd], ctx->cpu.gpr[rd]);
			break;

		case P_DISASM_TRACE_CPU_LO:
			fmt("LO=0x%08X", ctx->cpu.lo);
			break;

		case P_DISASM_TRACE_CPU_HI:
			fmt("HI=0x%08X", ctx->cpu.hi);
			break;

		case P_DISASM_TRACE_COUNT:
		default:
			assert(false);
		}
	}

#undef fmt
#undef rt
#undef rd
}
