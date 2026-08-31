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

#define TRACE_NUM_SPACES (40)

static const char *gpr[P_GPR_COUNT] = {
	[P_ZERO] = "$zero", [P_AT] = "$at", [P_V0] = "$v0", [P_V1] = "$v1",
	[P_A0] = "$a0",	    [P_A1] = "$a1", [P_A2] = "$a2", [P_A3] = "$a3",
	[P_T0] = "$t0",	    [P_T1] = "$t1", [P_T2] = "$t2", [P_T3] = "$t3",
	[P_T4] = "$t4",	    [P_T5] = "$t5", [P_T6] = "$t6", [P_T7] = "$t7",
	[P_S0] = "$s0",	    [P_S1] = "$s1", [P_S2] = "$s2", [P_S3] = "$s3",
	[P_S4] = "$s4",	    [P_S5] = "$s5", [P_S6] = "$s6", [P_S7] = "$s7",
	[P_T8] = "$t8",	    [P_T9] = "$t9", [P_K0] = "$k0", [P_K1] = "$k1",
	[P_GP] = "$gp",	    [P_SP] = "$sp", [P_FP] = "$fp", [P_RA] = "$ra"
};

static const char *cop0[P_COP0_COUNT] = {
	[0] = "REG0",	   [1] = "REG1",	[2] = "REG2",
	[P_BPC] = "BPC",   [4] = "REG4",	[P_BDA] = "BDA",
	[P_TAR] = "TAR",   [P_DCIC] = "DCIC",	[P_BADVADDR] = "BADVADDR",
	[P_BDAM] = "BDAM", [10] = "REG10",	[P_BPCM] = "BPCM",
	[P_SR] = "SR",	   [P_CAUSE] = "CAUSE", [P_EPC] = "EPC",
	[P_PRID] = "PRID", [16] = "REG16",	[17] = "REG17",
	[18] = "REG18",	   [19] = "REG19",	[20] = "REG20",
	[21] = "REG21",	   [22] = "REG22",	[23] = "REG23",
	[24] = "REG24",	   [25] = "REG25",	[26] = "REG26",
	[27] = "REG27",	   [28] = "REG28",	[29] = "REG29",
	[30] = "REG30",	   [31] = "REG31"
};

static const char *cop2_cpr[P_COP2_CPR_CNT] = {
	[P_VXY0] = "VXY0", [P_VZ0] = "VZ0",   [P_VXY1] = "VXY1",
	[P_VZ1] = "VZ1",   [P_RGBC] = "RGBC", [P_OTZ] = "OTZ",
	[P_IR0] = "IR0",   [P_IR1] = "IR1",   [P_IR2] = "IR2",
	[P_IR3] = "IR3",   [P_SXY0] = "SXY0", [P_SXY1] = "SXY1",
	[P_SXY2] = "SXY2", [P_SXYP] = "SXYP", [P_SZ0] = "SZ0",
	[P_SZ1] = "SZ1",   [P_SZ2] = "SZ2",   [P_SZ3] = "SZ3",
	[P_RGB0] = "RGB0", [P_RGB1] = "RGB1", [P_RGB2] = "RGB2",
	[P_RES1] = "RES1", [P_MAC0] = "MAC0", [P_MAC1] = "MAC1",
	[P_MAC2] = "MAC2", [P_MAC3] = "MAC3", [P_IRGB] = "IRGB",
	[P_ORGB] = "ORGB", [P_LZCS] = "LZCS", [P_LZCR] = "LZCR"
};

static const char *cop2_ccr[P_COP2_CCR_CNT] = {
	[P_R11R12] = "R11R12",
	[P_R13R21] = "R13R21"
};

P_NODISCARD P_NONNULL static u32 instr_get(struct p_ctx *ctx, u32 pc)
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

P_NODISCARD const char *p_gpr_get(enum p_cpu_gpr reg)
{
	assert(reg < P_GPR_COUNT);
	return gpr[reg];
}

P_NODISCARD const char *p_cop0_get(enum p_cpu_cop0 reg)
{
	assert(reg < P_COP0_COUNT);
	return cop0[reg];
}

void p_disasm_instr(struct p_ctx *ctx, u32 pc, struct p_disasm_traces *traces)
{
	p_str_init_fixed(&ctx->disasm.res.str, ctx->disasm.res.str_buf,
			 sizeof(ctx->disasm.res.str_buf));

	u32 instr = instr_get(ctx, pc);

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
#define simm	    ((s16)imm)
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

void p_disasm_trace_begin(struct p_ctx *ctx, u32 pc)
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
			u32 val = ctx->cpu.gpr_get(ctx, rt);

			fmt("%s=0x%08X", gpr[rt], val);
			break;
		}

		case P_DISASM_TRACE_GPR_RD: {
			u32 val = ctx->cpu.gpr_get(ctx, rd);
			fmt("%s=0x%08X", gpr[rd], val);

			break;
		}

		case P_DISASM_TRACE_CPU_LO:
			fmt("LO=0x%08X", ctx->cpu.lo_get(ctx));
			break;

		case P_DISASM_TRACE_CPU_HI:
			fmt("HI=0x%08X", ctx->cpu.hi_get(ctx));
			break;

		case P_DISASM_TRACE_CPU_PADDR: {
			u32 instr = ctx->cpu.instr_get(ctx);
			u32 vaddr = ctx->cpu.gpr_get(ctx, rs);

			u32 val = instr_imm(instr) + (vaddr & 0x1FFFFFFF);

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
