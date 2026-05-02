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
#include <stdio.h>

#include "psycho/compiler.h"
#include "psycho/ctx.h"

#include "bus.h"
#include "cpu-defs.h"

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

	[PSYCHO_CPU_COP0_BPC]		= "C0_BPC",
	[PSYCHO_CPU_COP0_BDA]		= "C0_BDA",
	[PSYCHO_CPU_COP0_TAR]		= "C0_TAR",
	[PSYCHO_CPU_COP0_DCIC]		= "C0_DCIC",
	[PSYCHO_CPU_COP0_BADVADDR]	= "C0_BADVADDR",
	[PSYCHO_CPU_COP0_BDAM]		= "C0_BDAM",
	[PSYCHO_CPU_COP0_BPCM]		= "C0_BPCM",
	[PSYCHO_CPU_COP0_SR]		= "C0_SR",
	[PSYCHO_CPU_COP0_CAUSE]		= "C0_CAUSE",
	[PSYCHO_CPU_COP0_EPC]		= "C0_EPC",
	[PSYCHO_CPU_COP0_PRID]		= "C0_PRID"

	// clang-format on
};

PSYCHO_NODISCARD static uint32_t instr_get(struct psycho_ctx *ctx, uint32_t pc)
{
	pc = cpu_vaddr_to_paddr(pc);
	return psycho_bus_load_word(ctx, pc);
}

PSYCHO_NODISCARD const char *
psycho_disasm_gpr_get(const enum psycho_cpu_gpr reg)
{
	assert(reg < PSYCHO_CPU_GPR_COUNT);
	return gpr[reg];
}

void psycho_disasm_instr(struct psycho_ctx *const ctx, char *const dst,
			 size_t *const len, const uint32_t pc)
{
	assert(ctx != NULL);
	assert(dst != NULL);
	assert(len != NULL);

#define op (cpu_instr_op(instr))
#define rt (cpu_instr_rt(instr))
#define rs (cpu_instr_rs(instr))
#define rd (cpu_instr_rd(instr))
#define shamt (cpu_instr_shamt(instr))
#define funct (cpu_instr_funct(instr))
#define imm (cpu_instr_imm(instr))
#define offset (imm)
#define base (rs)

	const uint32_t instr = instr_get(ctx, pc);

#define FORMAT(args...) *len = snprintf(dst, PSYCHO_DISASM_LEN_MAX, args)

	switch (op) {
	case CPU_INSTR_GROUP_SPECIAL:
		switch (funct) {
		case CPU_INSTR_SLL:
			FORMAT("sll %s, %s, 0x%X", gpr[rd], gpr[rt], shamt);
			return;

		case CPU_INSTR_ADDU:
			FORMAT("addu %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			return;

		case CPU_INSTR_OR:
			FORMAT("or %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			return;

		case CPU_INSTR_SLTU:
			FORMAT("sltu %s, %s, %s", gpr[rd], gpr[rs], gpr[rt]);
			return;

		default:
			break;
		}
		break;

	case CPU_INSTR_J:
		FORMAT("j 0x%08X", calc_jmp_addr(pc, instr));
		return;

	case CPU_INSTR_BNE:
		FORMAT("bne %s, %s, 0x%08X", gpr[rs], gpr[rt],
		       calc_branch_addr(pc, instr));
		return;

	case CPU_INSTR_ADDI:
		FORMAT("addi %s, %s, 0x%04X", gpr[rs], gpr[rs], imm);
		return;

	case CPU_INSTR_ADDIU:
		FORMAT("addiu %s, %s, 0x%04X", gpr[rt], gpr[rs], imm);
		return;

	case CPU_INSTR_ORI:
		FORMAT("ori %s, %s, 0x%04X", gpr[rt], gpr[rs], imm);
		return;

	case CPU_INSTR_LUI:
		FORMAT("lui %s, 0x%04X", gpr[rt], imm);
		return;

	case CPU_INSTR_GROUP_COP0:
		switch (rs) {
		case CPU_INSTR_MTC:
			FORMAT("mtc0 %s, %s", cop0[rd], gpr[rt]);
			return;

		default:
			break;
		}
		break;

	case CPU_INSTR_LW:
		FORMAT("lw %s, 0x%04X(%s)", gpr[rt], offset, gpr[base]);
		return;

	case CPU_INSTR_SH:
		FORMAT("sh %s, 0x%04X(%s)", gpr[rt], offset, gpr[base]);
		return;

	case CPU_INSTR_SW:
		FORMAT("sw %s, 0x%04X(%s)", gpr[rt], offset, gpr[base]);
		return;

	default:
		break;
	}

	FORMAT("illegal 0x%08X", instr);
}
