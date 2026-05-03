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
#include "cpu-defs.h"
#include "log.h"
#include "util.h"

LOG_MODULE(PSYCHO_LOG_MODULE_CPU);

static void illegal_instr(struct psycho_ctx *const ctx)
{
	assert(ctx != NULL);

	LOG_ERR(ctx, "illegal instruction trapped (pc=0x%08X, instr=0x%08X)",
		ctx->cpu.pc, ctx->cpu.instr);

	ctx->cpu.cfg.illegal_instr(ctx, ctx->cpu.instr);
}

PSYCHO_NODISCARD static uint32_t load_word(struct psycho_ctx *const ctx,
					   uint32_t vaddr)
{
	assert(ctx != NULL);

	vaddr = cpu_vaddr_to_paddr(vaddr);
	return psycho_bus_load_word(ctx, vaddr);
}

PSYCHO_NODISCARD static uint8_t load_byte(struct psycho_ctx *const ctx,
					  uint32_t vaddr)
{
	assert(ctx != NULL);

	vaddr = cpu_vaddr_to_paddr(vaddr);
	return psycho_bus_load_byte(ctx, vaddr);
}

static void store_word(struct psycho_ctx *const ctx, uint32_t vaddr,
		       const uint32_t word)
{
	if (ctx->cpu.cop0[PSYCHO_CPU_COP0_SR] & CPU_SR_ISC)
		return;

	vaddr = cpu_vaddr_to_paddr(vaddr);
	psycho_bus_store_word(ctx, vaddr, word);
}

static void store_halfword(struct psycho_ctx *const ctx, uint32_t vaddr,
			   const uint16_t halfword)
{
	if (ctx->cpu.cop0[PSYCHO_CPU_COP0_SR] & CPU_SR_ISC)
		return;

	vaddr = cpu_vaddr_to_paddr(vaddr);
	psycho_bus_store_halfword(ctx, vaddr, halfword);
}

static void store_byte(struct psycho_ctx *const ctx, uint32_t vaddr,
		       const uint8_t byte)
{
	if (ctx->cpu.cop0[PSYCHO_CPU_COP0_SR] & CPU_SR_ISC)
		return;

	vaddr = cpu_vaddr_to_paddr(vaddr);
	psycho_bus_store_byte(ctx, vaddr, byte);
}

static void disasm_trace(struct psycho_ctx *const ctx)
{
	assert(ctx != NULL);

	if (MODULE_LOG_LEVEL_ACTIVE(ctx, PSYCHO_LOG_LEVEL_TRACE)) {
		char result[PSYCHO_DISASM_LEN_MAX];
		size_t len;

		psycho_disasm_instr(ctx, result, &len, ctx->cpu.pc);
		LOG_TRACE_UNCHECKED(ctx, "[disasm] 0x%08X: %s", ctx->cpu.pc,
				    result);
	}
}

static void branch_if(struct psycho_ctx *const ctx, const bool condition_met)
{
	assert(ctx != NULL);

	if (condition_met)
		ctx->cpu.next_pc =
			calc_branch_addr(ctx->cpu.pc, ctx->cpu.instr);
}

void psycho_cpu_init(struct psycho_ctx *const ctx,
		     const struct psycho_cpu_cfg *const cfg)
{
	assert(ctx != NULL);
	assert(cfg != NULL);

	ctx->cpu.cfg = *cfg;
	LOG_INFO(ctx, "initialized");
}

void psycho_cpu_reset(struct psycho_ctx *const ctx)
{
	assert(ctx != NULL);

	memset(ctx->cpu.gpr, 0, sizeof(ctx->cpu.gpr));

	ctx->cpu.delay_pc = CPU_RESET_VECTOR;
	ctx->cpu.pc = CPU_RESET_VECTOR;
	ctx->cpu.next_pc = CPU_RESET_VECTOR + sizeof(ctx->cpu.instr);

	LOG_INFO(ctx, "reset");
}

void psycho_cpu_step(struct psycho_ctx *const ctx)
{
#define op (cpu_instr_op(ctx->cpu.instr))
#define rt (cpu_instr_rt(ctx->cpu.instr))
#define rs (cpu_instr_rs(ctx->cpu.instr))
#define rd (cpu_instr_rd(ctx->cpu.instr))
#define shamt (cpu_instr_shamt(ctx->cpu.instr))
#define funct (cpu_instr_funct(ctx->cpu.instr))
#define base (rs)
#define zextimm (zero_ext_16_32(cpu_instr_imm(ctx->cpu.instr)))
#define sextimm (sign_ext_16_32(cpu_instr_imm(ctx->cpu.instr)))
#define offset (sextimm)
#define gpr (ctx->cpu.gpr)

	assert(ctx != NULL);

	ctx->cpu.pc = ctx->cpu.delay_pc;
	ctx->cpu.instr = load_word(ctx, ctx->cpu.pc);

	ctx->cpu.delay_pc = ctx->cpu.next_pc;
	ctx->cpu.next_pc = ctx->cpu.delay_pc + sizeof(ctx->cpu.instr);

	disasm_trace(ctx);

	switch (op) {
	case CPU_INSTR_GROUP_SPECIAL:
		switch (funct) {
		case CPU_INSTR_SLL:
			gpr[rd] = gpr[rt] << shamt;
			break;

		case CPU_INSTR_SRL:
			gpr[rd] = gpr[rt] >> shamt;
			break;

		case CPU_INSTR_SRA:
			gpr[rd] = (int32_t)gpr[rt] >> shamt;
			break;

		case CPU_INSTR_JR:
			ctx->cpu.next_pc = gpr[rs];
			break;

		case CPU_INSTR_JALR:
			gpr[rd] = ctx->cpu.pc + (sizeof(ctx->cpu.instr) * 2);
			ctx->cpu.next_pc = gpr[rs];

			break;

		case CPU_INSTR_MFLO:
			gpr[rd] = ctx->cpu.lo;
			break;

		case CPU_INSTR_DIV:
			ctx->cpu.lo = gpr[rs] / gpr[rt];
			ctx->cpu.hi = gpr[rs] % gpr[rt];

			break;

		case CPU_INSTR_ADD:
		case CPU_INSTR_ADDU:
			gpr[rd] = gpr[rs] + gpr[rt];
			break;

		case CPU_INSTR_SUBU:
			gpr[rd] = gpr[rs] - gpr[rt];
			break;

		case CPU_INSTR_AND:
			gpr[rd] = gpr[rs] & gpr[rt];
			break;

		case CPU_INSTR_OR:
			gpr[rd] = gpr[rs] | gpr[rt];
			break;

		case CPU_INSTR_SLTU:
			gpr[rd] = gpr[rs] < gpr[rt];
			break;

		default:
			illegal_instr(ctx);
			return;
		}
		break;

	case CPU_INSTR_GROUP_REGIMM:
		switch (rt) {
		case CPU_INSTR_BLTZ:
			branch_if(ctx, (int32_t)gpr[rs] < 0);
			break;

		case CPU_INSTR_BGEZ:
			branch_if(ctx, (int32_t)gpr[rs] >= 0);
			break;

		default:
			illegal_instr(ctx);
			return;
		}
		break;

	case CPU_INSTR_J:
		ctx->cpu.next_pc = calc_jmp_addr(ctx->cpu.pc, ctx->cpu.instr);
		break;

	case CPU_INSTR_JAL:
		gpr[PSYCHO_CPU_REG_RA] =
			ctx->cpu.pc + (sizeof(ctx->cpu.instr) * 2);
		ctx->cpu.next_pc = calc_jmp_addr(ctx->cpu.pc, ctx->cpu.instr);

		break;

	case CPU_INSTR_BEQ:
		branch_if(ctx, gpr[rs] == gpr[rt]);
		break;

	case CPU_INSTR_BNE:
		branch_if(ctx, gpr[rs] != gpr[rt]);
		break;

	case CPU_INSTR_BLEZ:
		branch_if(ctx, (int32_t)gpr[rs] <= 0);
		break;

	case CPU_INSTR_BGTZ:
		branch_if(ctx, (int32_t)gpr[rs] > 0);
		break;

	case CPU_INSTR_ADDI:
	case CPU_INSTR_ADDIU:
		gpr[rt] = gpr[rs] + sextimm;
		break;

	case CPU_INSTR_SLTI:
		gpr[rt] = (int32_t)gpr[rs] < (int32_t)sextimm;
		break;

	case CPU_INSTR_ANDI:
		gpr[rt] = zextimm & gpr[rs];
		break;

	case CPU_INSTR_ORI:
		gpr[rt] = zextimm | gpr[rs];
		break;

	case CPU_INSTR_LUI:
		gpr[rt] = zextimm << 16;
		break;

	case CPU_INSTR_GROUP_COP0:
		switch (rs) {
		case CPU_INSTR_MFC:
			gpr[rt] = ctx->cpu.cop0[rd];
			break;

		case CPU_INSTR_MTC:
			ctx->cpu.cop0[rd] = gpr[rt];
			break;

		default:
			switch (funct) {
			default:
				illegal_instr(ctx);
				return;
			}
		}
		break;

	case CPU_INSTR_LB:
		gpr[rt] = sign_ext_8_32(load_byte(ctx, gpr[base] + offset));
		break;

	case CPU_INSTR_LW:
		gpr[rt] = load_word(ctx, gpr[base] + offset);
		break;

	case CPU_INSTR_LBU:
		gpr[rt] = zero_ext_8_32(load_byte(ctx, gpr[base] + offset));
		break;

	case CPU_INSTR_SB:
		store_byte(ctx, gpr[base] + offset, gpr[rt] & UINT8_MAX);
		break;

	case CPU_INSTR_SH:
		store_halfword(ctx, gpr[base] + offset, gpr[rt] & UINT16_MAX);
		break;

	case CPU_INSTR_SW:
		store_word(ctx, gpr[base] + offset, gpr[rt]);
		break;

	default:
		illegal_instr(ctx);
		return;
	}
}
