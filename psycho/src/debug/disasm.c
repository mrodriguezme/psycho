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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "psycho/common/compiler_support.h"
#include "psycho/ctx.h"

#include "system/bus.h"
#include "system/cpu/defs.h"
#include "disasm.h"
#include "str.h"

#define TRACE_NUM_SPACES (40)

static const char *const gpr[PSYCHO_CPU_GPR_COUNT] = {
	// clang-format off

	[PSYCHO_CPU_GPR_ZERO]	= "$zero",
	[PSYCHO_CPU_GPR_AT]	= "$at",
	[PSYCHO_CPU_GPR_V0]	= "$v0",
	[PSYCHO_CPU_GPR_V1]	= "$v1",
	[PSYCHO_CPU_GPR_A0]	= "$a0",
	[PSYCHO_CPU_GPR_A1]	= "$a1",
	[PSYCHO_CPU_GPR_A2]	= "$a2",
	[PSYCHO_CPU_GPR_A3]	= "$a3",
	[PSYCHO_CPU_GPR_T0]	= "$t0",
	[PSYCHO_CPU_GPR_T1]	= "$t1",
	[PSYCHO_CPU_GPR_T2]	= "$t2",
	[PSYCHO_CPU_GPR_T3]	= "$t3",
	[PSYCHO_CPU_GPR_T4]	= "$t4",
	[PSYCHO_CPU_GPR_T5]	= "$t5",
	[PSYCHO_CPU_GPR_T6]	= "$t6",
	[PSYCHO_CPU_GPR_T7]	= "$t7",
	[PSYCHO_CPU_GPR_S0]	= "$s0",
	[PSYCHO_CPU_GPR_S1]	= "$s1",
	[PSYCHO_CPU_GPR_S2]	= "$s2",
	[PSYCHO_CPU_GPR_S3]	= "$s3",
	[PSYCHO_CPU_GPR_S4]	= "$s4",
	[PSYCHO_CPU_GPR_S5]	= "$s5",
	[PSYCHO_CPU_GPR_S6]	= "$s6",
	[PSYCHO_CPU_GPR_S7]	= "$s7",
	[PSYCHO_CPU_GPR_T8]	= "$t8",
	[PSYCHO_CPU_GPR_T9]	= "$t9",
	[PSYCHO_CPU_GPR_K0]	= "$k0",
	[PSYCHO_CPU_GPR_K1]	= "$k1",
	[PSYCHO_CPU_GPR_GP]	= "$gp",
	[PSYCHO_CPU_GPR_SP]	= "$sp",
	[PSYCHO_CPU_GPR_FP]	= "$fp",
	[PSYCHO_CPU_GPR_RA]	= "$ra"

	// clang-format on
};

static const char *const cop0_cpr[PSYCHO_CPU_COP0_CPR_COUNT] = {
	// clang-format off

	[0]				= "C0_REG0",
	[1]				= "C0_REG1",
	[2]				= "C0_REG2",
	[PSYCHO_CPU_COP0_CPR_BPC]	= "C0_BPC",
	[4]				= "C0_REG4",
	[PSYCHO_CPU_COP0_CPR_BDA]	= "C0_BDA",
	[PSYCHO_CPU_COP0_CPR_TAR]	= "C0_TAR",
	[PSYCHO_CPU_COP0_CPR_DCIC]	= "C0_DCIC",
	[PSYCHO_CPU_COP0_CPR_BADVADDR]	= "C0_BADVADDR",
	[PSYCHO_CPU_COP0_CPR_BDAM]	= "C0_BDAM",
	[10]				= "C0_REG10",
	[PSYCHO_CPU_COP0_CPR_BPCM]	= "C0_BPCM",
	[PSYCHO_CPU_COP0_CPR_SR]	= "C0_SR",
	[PSYCHO_CPU_COP0_CPR_CAUSE]	= "C0_CAUSE",
	[PSYCHO_CPU_COP0_CPR_EPC]	= "C0_EPC",
	[PSYCHO_CPU_COP0_CPR_PRID]	= "C0_PRID",
	[16]				= "C0_REG16",
	[17]				= "C0_REG17",
	[18]				= "C0_REG18",
	[19]				= "C0_REG19",
	[20]				= "C0_REG20",
	[21]				= "C0_REG21",
	[22]				= "C0_REG22",
	[23]				= "C0_REG23",
	[24]				= "C0_REG24",
	[25]				= "C0_REG25",
	[26]				= "C0_REG26",
	[27]				= "C0_REG27",
	[28]				= "C0_REG28",
	[29]				= "C0_REG29",
	[30]				= "C0_REG30",
	[31]				= "C0_REG31"

	// clang-format on
};

static const char *const cop2_cpr[PSYCHO_CPU_COP2_CPR_COUNT] = {
	// clang-format off

	[PSYCHO_CPU_COP2_CPR_VXY0]	= "C2_VXY0",
	[PSYCHO_CPU_COP2_CPR_VZ0]	= "C2_VZ0",
	[PSYCHO_CPU_COP2_CPR_VXY1]	= "C2_VXY1",
	[PSYCHO_CPU_COP2_CPR_VZ1]	= "C2_VZ1",
	[PSYCHO_CPU_COP2_CPR_RGBC]	= "C2_RGBC",
	[PSYCHO_CPU_COP2_CPR_OTZ]	= "C2_OTZ",
	[PSYCHO_CPU_COP2_CPR_IR0]	= "C2_IR0",
	[PSYCHO_CPU_COP2_CPR_IR1]	= "C2_IR1",
	[PSYCHO_CPU_COP2_CPR_IR2]	= "C2_IR2",
	[PSYCHO_CPU_COP2_CPR_IR3]	= "C2_IR3",
	[PSYCHO_CPU_COP2_CPR_SXY0]	= "C2_SXY0",
	[PSYCHO_CPU_COP2_CPR_SXY1]	= "C2_SXY1",
	[PSYCHO_CPU_COP2_CPR_SXY2]	= "C2_SXY2",
	[PSYCHO_CPU_COP2_CPR_SXYP]	= "C2_SXYP",
	[PSYCHO_CPU_COP2_CPR_SZ0]	= "C2_SZ0",
	[PSYCHO_CPU_COP2_CPR_SZ1]	= "C2_SZ1",
	[PSYCHO_CPU_COP2_CPR_SZ2]	= "C2_SZ2",
	[PSYCHO_CPU_COP2_CPR_SZ3]	= "C2_SZ3",
	[PSYCHO_CPU_COP2_CPR_RGB0]	= "C2_RGB0",
	[PSYCHO_CPU_COP2_CPR_RGB1]	= "C2_RGB1",
	[PSYCHO_CPU_COP2_CPR_RGB2]	= "C2_RGB2",
	[PSYCHO_CPU_COP2_CPR_RES1]	= "C2_RES1",
	[PSYCHO_CPU_COP2_CPR_MAC0]	= "C2_MAC0",
	[PSYCHO_CPU_COP2_CPR_MAC1]	= "C2_MAC1",
	[PSYCHO_CPU_COP2_CPR_MAC2]	= "C2_MAC2",
	[PSYCHO_CPU_COP2_CPR_MAC3]	= "C2_MAC3",
	[PSYCHO_CPU_COP2_CPR_IRGB]	= "C2_IRGB",
	[PSYCHO_CPU_COP2_CPR_ORGB]	= "C2_ORGB",
	[PSYCHO_CPU_COP2_CPR_LZCS]	= "C2_LZCS",
	[PSYCHO_CPU_COP2_CPR_LZCR]	= "C2_LZCR"

	// clang-format on
};

static const char *const cop2_ccr[PSYCHO_CPU_COP2_CCR_COUNT] = {
	// clang-format off

	[PSYCHO_CPU_COP2_CCR_R11R12]	= "C2_R11R12",
	[PSYCHO_CPU_COP2_CCR_R13R21]	= "C2_R13R21",
	[PSYCHO_CPU_COP2_CCR_R22R23]	= "C2_R22R23",
	[PSYCHO_CPU_COP2_CCR_R31R32]	= "C2_R31R32",
	[PSYCHO_CPU_COP2_CCR_R33]	= "C2_R33",
	[PSYCHO_CPU_COP2_CCR_TRX]	= "C2_TRX",
	[PSYCHO_CPU_COP2_CCR_TRY]	= "C2_TRY",
	[PSYCHO_CPU_COP2_CCR_TRZ]	= "C2_TRZ",
	[PSYCHO_CPU_COP2_CCR_L11L12]	= "C2_L11L12",
	[PSYCHO_CPU_COP2_CCR_L13L21]	= "C2_L13L21",
	[PSYCHO_CPU_COP2_CCR_L22L23]	= "C2_L22L23",
	[PSYCHO_CPU_COP2_CCR_L31L32]	= "C2_L31L32",
	[PSYCHO_CPU_COP2_CCR_L33]	= "C2_L33",
	[PSYCHO_CPU_COP2_CCR_RBK]	= "C2_RBK",
	[PSYCHO_CPU_COP2_CCR_GBK]	= "C2_GBK",
	[PSYCHO_CPU_COP2_CCR_BBK]	= "C2_BBK",
	[PSYCHO_CPU_COP2_CCR_LR1LR2]	= "C2_LR1LR2",
	[PSYCHO_CPU_COP2_CCR_LR3LG1]	= "C2_LR3LG1",
	[PSYCHO_CPU_COP2_CCR_LG2LG3]	= "C2_LG2LG3",
	[PSYCHO_CPU_COP2_CCR_LB1LB2]	= "C2_LB1LB2",
	[PSYCHO_CPU_COP2_CCR_LB3]	= "C2_LB3",
	[PSYCHO_CPU_COP2_CCR_RFC]	= "C2_RFC",
	[PSYCHO_CPU_COP2_CCR_GFC]	= "C2_GFC",
	[PSYCHO_CPU_COP2_CCR_BFC]	= "C2_BFC",
	[PSYCHO_CPU_COP2_CCR_OFX]	= "C2_OFX",
	[PSYCHO_CPU_COP2_CCR_OFY]	= "C2_OFY",
	[PSYCHO_CPU_COP2_CCR_H]		= "C2_H",
	[PSYCHO_CPU_COP2_CCR_DQA]	= "C2_DQA",
	[PSYCHO_CPU_COP2_CCR_DQB]	= "C2_DQB",
	[PSYCHO_CPU_COP2_CCR_ZSF3]	= "C2_ZSF3",
	[PSYCHO_CPU_COP2_CCR_ZSF4]	= "C2_ZSF4",
	[PSYCHO_CPU_COP2_CCR_FLAG]	= "C2_FLAG"

	// clang-format on
};

P_NODISCARD P_NONNULL static uint32_t instr_get(struct p_ctx *ctx, uint32_t pc)
{
	pc = vaddr_to_paddr(pc);
	return p_load32(ctx, pc);
}

static void trace_add(struct p_disasm_traces *traces, enum p_disasm_trace trace)
{
	if (traces) {
		assert(traces->count < ARRAY_SIZE(traces->data));
		traces->data[traces->count++] = trace;
	}
}

P_NODISCARD const char *p_gpr_get(enum psycho_cpu_gpr reg)
{
	assert(reg < PSYCHO_CPU_GPR_COUNT);
	return gpr[reg];
}

P_NODISCARD const char *p_cop0_get(enum psycho_cpu_cop0_cpr reg)
{
	assert(reg < PSYCHO_CPU_COP0_CPR_COUNT);
	return cop0_cpr[reg];
}

void p_disasm_instr(struct p_ctx *ctx, uint32_t pc, struct p_disasm_traces *traces)
{
	p_str_init_fixed(&ctx->disasm.res.str, ctx->disasm.res.str_buf, sizeof(ctx->disasm.res.str_buf));

	uint32_t instr = instr_get(ctx, pc);

	ctx->disasm.res.instr = instr;
	ctx->disasm.res.pc    = pc;

#define fmt(args...)                                                  \
	({                                                            \
		bool truncated;                                       \
		p_str_append(&ctx->disasm.res.str, &truncated, args); \
		assert(!truncated);                                   \
	})

#define op	    (instr_op(instr))
#define rt	    (instr_rt(instr))
#define rs	    (instr_rs(instr))
#define rd	    (instr_rd(instr))
#define shamt	    (instr_shamt(instr))
#define funct	    (instr_funct(instr))
#define imm	    (instr_imm(instr))
#define simm	    ((int16_t)imm)
#define offset	    (simm)
#define base	    (rs)
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

		case SYSCALL:
			fmt("syscall");
			return;

		case BREAK:
			fmt("break");
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

			return;

		case MULTU:
			fmt("multu %s, %s", gpr[rs], gpr[rt]);
			trace_add(traces, P_DISASM_TRACE_CPU_LO);
			trace_add(traces, P_DISASM_TRACE_CPU_HI);

			return;

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

	case GRP_REGIMM: {
		const char *link = ((rt & 0x1E) == 0x10) ? "al" : "";
		const char *name = (rt & 1) ? "bgez" : "bltz";

		fmt("%s%s %s, 0x%08X", name, link, gpr[rs], branch_addr);
		return;
	}

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
			fmt("mfc0 %s, %s", gpr[rt], cop0_cpr[rd]);
			trace_add(traces, P_DISASM_TRACE_GPR_RT);

			return;

		case MTC:
			fmt("mtc0 %s, %s", cop0_cpr[rd], gpr[rt]);
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

	case GRP_COP2:
		switch (rs) {
		case MTC:
			fmt("mtc2 %s, %s", gpr[rt], cop2_cpr[rd]);
			return;

		case CTC:
			fmt("ctc2 %s, %s", gpr[rt], cop2_ccr[rd]);
			return;

		default:
			break;
		}
		break;

	case LB:
		fmt("lb %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

		return;

	case LH:
		fmt("lh %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

		return;

	case LWL:
		fmt("lwl %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

		return;

	case LW:
		fmt("lw %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

		return;

	case LBU:
		fmt("lbu %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

		return;

	case LHU:
		fmt("lhu %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

		return;

	case LWR:
		fmt("lwr %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_GPR_RT);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

		return;

	case SB:
		fmt("sb %s, %d(%s)", gpr[rt], offset, gpr[base]);

		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);
		return;

	case SH:
		fmt("sh %s, %d(%s)", gpr[rt], offset, gpr[base]);

		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);
		return;

	case SWL:
		fmt("swl %s, %d(%s)", gpr[rt], offset, gpr[base]);

		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);
		return;

	case SW:
		fmt("sw %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

		return;

	case SWR:
		fmt("swr %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, P_DISASM_TRACE_CPU_PADDR);

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

void p_disasm_trace_begin(struct p_ctx *ctx, uint32_t pc)
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

void p_disasm_trace_end(struct p_ctx *ctx)
{
#define fmt(args...)                                                  \
	({                                                            \
		bool truncated;                                       \
		p_str_append(&ctx->disasm.res.str, &truncated, args); \
		assert(!truncated);                                   \
	})

#define rt (instr_rt(ctx->disasm.res.instr))
#define rd (instr_rd(ctx->disasm.res.instr))
#define rs (instr_rs(ctx->disasm.res.instr))

	for (size_t trace = 0; trace < ctx->disasm.traces.count; ++trace) {
		if (trace)
			fmt(", ");

		switch (ctx->disasm.traces.data[trace]) {
		case P_DISASM_TRACE_GPR_RT: {
			uint32_t val = ctx->cpu.gpr_get(ctx, rt);

			fmt("%s=0x%08X", gpr[rt], val);
			break;
		}

		case P_DISASM_TRACE_GPR_RD: {
			uint32_t val = ctx->cpu.gpr_get(ctx, rd);
			fmt("%s=0x%08X", gpr[rd], val);

			break;
		}

		case P_DISASM_TRACE_CPU_LO:
			fmt("LO=0x%08X", ctx->cpu.reg_lo_get(ctx));
			break;

		case P_DISASM_TRACE_CPU_HI:
			fmt("HI=0x%08X", ctx->cpu.reg_hi_get(ctx));
			break;

		case P_DISASM_TRACE_CPU_PADDR: {
			uint32_t instr = ctx->cpu.instr_get(ctx);
			uint32_t vaddr = ctx->cpu.gpr_get(ctx, rs);

			uint32_t val = instr_imm(instr) + (vaddr & 0x1FFFFFFF);

			fmt("paddr=0x%08X", val);
			break;
		}

		case P_DISASM_TRACE_COUNT:
		default:
			assert(false);
		}
	}

#undef fmt
#undef rt
#undef rd
}
