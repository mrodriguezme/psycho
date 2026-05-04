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
#include "cpu-defs.h"
#include "disasm.h"
#include "types.h"

enum {
	TRACE_NUM_SPACES = 40,
};

static const char *const gpr[PSYCHO_CPU_GPR_COUNT] = {
	// clang-format off

	[PSYCHO_CPU_REG_ZERO]	= "$zero",
	[PSYCHO_CPU_REG_AT]	= "$at",
	[PSYCHO_CPU_REG_V0]	= "$v0",
	[PSYCHO_CPU_REG_V1]	= "$v1",
	[PSYCHO_CPU_REG_A0]	= "$a0",
	[PSYCHO_CPU_REG_A1]	= "$a1",
	[PSYCHO_CPU_REG_A2]	= "$a2",
	[PSYCHO_CPU_REG_A3]	= "$a3",
	[PSYCHO_CPU_REG_T0]	= "$t0",
	[PSYCHO_CPU_REG_T1]	= "$t1",
	[PSYCHO_CPU_REG_T2]	= "$t2",
	[PSYCHO_CPU_REG_T3]	= "$t3",
	[PSYCHO_CPU_REG_T4]	= "$t4",
	[PSYCHO_CPU_REG_T5]	= "$t5",
	[PSYCHO_CPU_REG_T6]	= "$t6",
	[PSYCHO_CPU_REG_T7]	= "$t7",
	[PSYCHO_CPU_REG_S0]	= "$s0",
	[PSYCHO_CPU_REG_S1]	= "$s1",
	[PSYCHO_CPU_REG_S2]	= "$s2",
	[PSYCHO_CPU_REG_S3]	= "$s3",
	[PSYCHO_CPU_REG_S4]	= "$s4",
	[PSYCHO_CPU_REG_S5]	= "$s5",
	[PSYCHO_CPU_REG_S6]	= "$s6",
	[PSYCHO_CPU_REG_S7]	= "$s7",
	[PSYCHO_CPU_REG_T8]	= "$t8",
	[PSYCHO_CPU_REG_T9]	= "$t9",
	[PSYCHO_CPU_REG_K0]	= "$k0",
	[PSYCHO_CPU_REG_K1]	= "$k1",
	[PSYCHO_CPU_REG_GP]	= "$gp",
	[PSYCHO_CPU_REG_SP]	= "$sp",
	[PSYCHO_CPU_REG_FP]	= "$fp",
	[PSYCHO_CPU_REG_RA]	= "$ra"

	// clang-format on
};

static const char *const cop0[PSYCHO_CPU_COP0_COUNT] = {
	// clang-format off

	[0]				= "C0_UNUSED0",
	[1]				= "C0_UNUSED1",
	[2]				= "C0_UNUSED2",
	[PSYCHO_CPU_COP0_BPC]		= "C0_BPC",
	[4]				= "C0_UNUSED4",
	[PSYCHO_CPU_COP0_BDA]		= "C0_BDA",
	[PSYCHO_CPU_COP0_TAR]		= "C0_TAR",
	[PSYCHO_CPU_COP0_DCIC]		= "C0_DCIC",
	[PSYCHO_CPU_COP0_BADVADDR]	= "C0_BADVADDR",
	[PSYCHO_CPU_COP0_BDAM]		= "C0_BDAM",
	[10]				= "C0_UNUSED10",
	[PSYCHO_CPU_COP0_BPCM]		= "C0_BPCM",
	[PSYCHO_CPU_COP0_SR]		= "C0_SR",
	[PSYCHO_CPU_COP0_CAUSE]		= "C0_CAUSE",
	[PSYCHO_CPU_COP0_EPC]		= "C0_EPC",
	[PSYCHO_CPU_COP0_PRID]		= "C0_PRID",
	[16]				= "C0_UNUSED16",
	[17]				= "C0_UNUSED17",
	[18]				= "C0_UNUSED18",
	[19]				= "C0_UNUSED19",
	[20]				= "C0_UNUSED20",
	[21]				= "C0_UNUSED21",
	[22]				= "C0_UNUSED22",
	[23]				= "C0_UNUSED23",
	[24]				= "C0_UNUSED24",
	[25]				= "C0_UNUSED25",
	[26]				= "C0_UNUSED26",
	[27]				= "C0_UNUSED27",
	[28]				= "C0_UNUSED28",
	[29]				= "C0_UNUSED29",
	[30]				= "C0_UNUSED30",
	[31]				= "C0_UNUSED31"

	// clang-format on
};

__attribute__((format(printf, 3, 4))) static void
append(char *const buf, size_t *const len, const char *const fmt, ...)
{
	const size_t rem = PSYCHO_DISASM_LEN_MAX - *len;

	va_list ap;
	va_start(ap, fmt);
	const int written = vsnprintf(&buf[*len], rem, fmt, ap);
	va_end(ap);

	if (written) {
		if ((size_t)written >= rem)
			*len = PSYCHO_DISASM_LEN_MAX - 1;
		else
			*len += (size_t)written;
	}
}

PSYCHO_NODISCARD static u32 instr_get(struct psycho_ctx *const ctx, u32 pc)
{
	pc = cpu_vaddr_to_paddr(pc);
	return psycho_bus_load_word(ctx, pc);
}

static void trace_add(struct psycho_disasm_traces *const traces,
		      const enum psycho_disasm_trace trace)
{
	if (traces) {
		assert(traces->count < ARRAY_SIZE(traces->data));
		traces->data[traces->count++] = trace;
	}
}

PSYCHO_NODISCARD const char *
psycho_disasm_gpr_get(const enum psycho_cpu_gpr reg)
{
	assert(reg < PSYCHO_CPU_GPR_COUNT);
	return gpr[reg];
}

PSYCHO_NODISCARD const char *
psycho_disasm_cop0_get(const enum psycho_cpu_cop0 reg)
{
	assert(reg < PSYCHO_CPU_COP0_COUNT);
	return cop0[reg];
}

void psycho_disasm_init(struct psycho_ctx *const ctx,
			const struct psycho_disasm_cfg *const cfg)
{
	ctx->disasm.cfg = *cfg;
}

void psycho_disasm_instr(struct psycho_ctx *const ctx, const u32 pc,
			 struct psycho_disasm_traces *traces)
{
	assert(ctx != NULL);

	memset(&ctx->disasm.res, 0, sizeof(ctx->disasm.res));

	const u32 instr = instr_get(ctx, pc);

	ctx->disasm.res.instr = instr;
	ctx->disasm.res.pc = pc;

#define fmt(args...) append(ctx->disasm.res.str, &ctx->disasm.res.len, args)

#define op (cpu_instr_op(instr))
#define rt (cpu_instr_rt(instr))
#define rs (cpu_instr_rs(instr))
#define rd (cpu_instr_rd(instr))
#define shamt (cpu_instr_shamt(instr))
#define funct (cpu_instr_funct(instr))
#define imm (cpu_instr_imm(instr))
#define simm ((s16)imm)
#define offset (simm)
#define base (rs)
#define branch_addr (cpu_branch_addr(pc, instr))

	switch (op) {
	case CPU_INSTR_GROUP_SPECIAL:
		switch (funct) {
		case CPU_INSTR_SLL:
			fmt("sll %s, %s, 0x%X", gpr[rd], gpr[rt], shamt);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_SRL:
			fmt("srl %s, %s, %u", gpr[rd], gpr[rt], shamt);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_SRA:
			fmt("sra %s, %s, %u", gpr[rd], gpr[rt], shamt);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_JR:
			fmt("jr %s", gpr[rs]);
			return;

		case CPU_INSTR_JALR:
			fmt("jalr %s, %s", gpr[rd], gpr[rs]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_MFHI:
			fmt("mfhi %s", gpr[rd]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_MFLO:
			fmt("mflo %s", gpr[rd]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_DIV:
			fmt("div %s, %s", gpr[rs], gpr[rt]);
			trace_add(traces, PSYCHO_DISASM_TRACE_CPU_LO);
			trace_add(traces, PSYCHO_DISASM_TRACE_CPU_HI);

			return;

		case CPU_INSTR_DIVU:
			fmt("divu %s, %s", gpr[rs], gpr[rt]);
			trace_add(traces, PSYCHO_DISASM_TRACE_CPU_LO);
			trace_add(traces, PSYCHO_DISASM_TRACE_CPU_HI);

			return;

		case CPU_INSTR_ADD:
			fmt("add %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);

			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);
			return;

		case CPU_INSTR_ADDU:
			fmt("addu %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_SUBU:
			fmt("subu %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_AND:
			fmt("and %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_OR:
			fmt("or %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_SLT:
			fmt("slt %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);

			return;

		case CPU_INSTR_SLTU:
			fmt("sltu %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);

			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RD);
			return;

		default:
			break;
		}
		break;

	case CPU_INSTR_GROUP_REGIMM:
		switch (rt) {
		case CPU_INSTR_BLTZ:
			fmt("bltz %s, 0x%08X", gpr[rs], branch_addr);
			return;

		case CPU_INSTR_BGEZ:
			fmt("bgez %s, 0x%08X", gpr[rs], branch_addr);
			return;

		default:
			break;
		}
		break;

	case CPU_INSTR_J:
		fmt("j 0x%08X", cpu_jmp_addr(pc, instr));
		return;

	case CPU_INSTR_JAL:
		fmt("jal 0x%08X", cpu_jmp_addr(pc, instr));
		return;

	case CPU_INSTR_BEQ:
		fmt("beq %s, %s, 0x%08X", gpr[rs], gpr[rt], branch_addr);
		return;

	case CPU_INSTR_BNE:
		fmt("bne %s, %s, 0x%08X", gpr[rs], gpr[rt], branch_addr);
		return;

	case CPU_INSTR_BLEZ:
		fmt("blez %s, 0x%08X", gpr[rs], branch_addr);
		return;

	case CPU_INSTR_BGTZ:
		fmt("bgtz %s, 0x%08X", gpr[rs], branch_addr);
		return;

	case CPU_INSTR_ADDI:
		fmt("addi %s, %s, %d", gpr[rt], gpr[rs], simm);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_ADDIU:
		fmt("addiu %s, %s, %d", gpr[rt], gpr[rs], simm);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_SLTI:
		fmt("slti %s, %s, %d", gpr[rt], gpr[rs], simm);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_SLTIU:
		fmt("sltiu %s, %s, %d", gpr[rt], gpr[rs], simm);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_ANDI:
		fmt("andi %s, %s, 0x%04X", gpr[rt], gpr[rs], imm);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_ORI:
		fmt("ori %s, %s, 0x%04X", gpr[rt], gpr[rs], imm);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_LUI:
		fmt("lui %s, 0x%04X", gpr[rt], imm);

		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);
		return;

	case CPU_INSTR_GROUP_COP0:
		switch (rs) {
		case CPU_INSTR_MFC:
			fmt("mfc0 %s, %s", gpr[rt], cop0[rd]);
			trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

			return;

		case CPU_INSTR_MTC:
			fmt("mtc0 %s, %s", cop0[rd], gpr[rt]);
			return;

		default:
			break;
		}
		break;

	case CPU_INSTR_LB:
		fmt("lb %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_LW:
		fmt("lw %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_LBU:
		fmt("lbu %s, %d(%s)", gpr[rt], offset, gpr[base]);
		trace_add(traces, PSYCHO_DISASM_TRACE_GPR_RT);

		return;

	case CPU_INSTR_SB:
		fmt("sb %s, %d(%s)", gpr[rt], offset, gpr[base]);
		return;

	case CPU_INSTR_SH:
		fmt("sh %s, %d(%s)", gpr[rt], offset, gpr[base]);
		return;

	case CPU_INSTR_SW:
		fmt("sw %s, %d(%s)", gpr[rt], offset, gpr[base]);
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

void psycho_disasm_trace_begin(struct psycho_ctx *const ctx, const u32 pc)
{
	memset(&ctx->disasm.traces, 0, sizeof(ctx->disasm.traces));
	psycho_disasm_instr(ctx, pc, &ctx->disasm.traces);

	if (!ctx->disasm.traces.count)
		return;

	if (ctx->disasm.res.len < TRACE_NUM_SPACES) {
		const size_t pad = TRACE_NUM_SPACES - ctx->disasm.res.len;

		memset(&ctx->disasm.res.str[ctx->disasm.res.len], ' ', pad);
		ctx->disasm.res.len += pad;
	}
	append(ctx->disasm.res.str, &ctx->disasm.res.len, "; ");
}

void psycho_disasm_trace_end(struct psycho_ctx *const ctx)
{
#define fmt(args...) append(ctx->disasm.res.str, &ctx->disasm.res.len, args)
#define rt (cpu_instr_rt(ctx->disasm.res.instr))
#define rd (cpu_instr_rd(ctx->disasm.res.instr))

	for (size_t trace = 0; trace < ctx->disasm.traces.count; ++trace) {
		if (trace)
			fmt(", ");

		switch (ctx->disasm.traces.data[trace]) {
		case PSYCHO_DISASM_TRACE_GPR_RT:
			fmt("%s=0x%08X", gpr[rt], ctx->cpu.gpr[rt]);
			break;

		case PSYCHO_DISASM_TRACE_GPR_RD:
			fmt("%s=0x%08X", gpr[rd], ctx->cpu.gpr[rd]);
			break;

		case PSYCHO_DISASM_TRACE_CPU_LO:
			fmt("LO=0x%08X", ctx->cpu.lo);
			break;

		case PSYCHO_DISASM_TRACE_CPU_HI:
			fmt("HI=0x%08X", ctx->cpu.hi);
			break;

		case PSYCHO_DISASM_TRACE_COUNT:
		default:
			assert(false);
		}
	}

#undef fmt
#undef rt
#undef rd
}
