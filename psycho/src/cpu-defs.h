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

#pragma once

#include <stdint.h>
#include "psycho/compiler.h"
#include "util.h"

#define CPU_RESET_VECTOR (UINT32_C(0xBFC00000))

enum psycho_cpu_instr_op {
	CPU_INSTR_GROUP_SPECIAL = 0x00,
	CPU_INSTR_J = 0x02,
	CPU_INSTR_JAL = 0x03,
	CPU_INSTR_BEQ = 0x04,
	CPU_INSTR_BNE = 0x05,
	CPU_INSTR_ADDI = 0x08,
	CPU_INSTR_ADDIU = 0x09,
	CPU_INSTR_ANDI = 0x0C,
	CPU_INSTR_ORI = 0x0D,
	CPU_INSTR_LUI = 0x0F,
	CPU_INSTR_GROUP_COP0 = 0x10,
	CPU_INSTR_LB = 0x20,
	CPU_INSTR_LW = 0x23,
	CPU_INSTR_SB = 0x28,
	CPU_INSTR_SH = 0x29,
	CPU_INSTR_SW = 0x2B
};

enum psycho_cpu_instr_special {
	CPU_INSTR_SLL = 0x00,
	CPU_INSTR_JR = 0x08,
	CPU_INSTR_ADDU = 0x21,
	CPU_INSTR_AND = 0x24,
	CPU_INSTR_OR = 0x25,
	CPU_INSTR_SLTU = 0x2B
};

enum psycho_cpu_instr_cop {
	CPU_INSTR_MFC = 0x00,
	CPU_INSTR_MTC = 0x04,
};

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint32_t
cpu_vaddr_to_paddr(const uint32_t vaddr)
{
	return vaddr & 0x1FFFFFFF;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE unsigned int
cpu_instr_op(const uint32_t instr)
{
	return instr >> 26;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE unsigned int
cpu_instr_rs(const uint32_t instr)
{
	return (instr >> 21) & 0x1F;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE unsigned int
cpu_instr_rt(const uint32_t instr)
{
	return (instr >> 16) & 0x1F;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE unsigned int
cpu_instr_rd(const uint32_t instr)
{
	return (instr >> 11) & 0x1F;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE unsigned int
cpu_instr_shamt(const uint32_t instr)
{
	return (instr >> 6) & 0x1F;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE unsigned int
cpu_instr_funct(const uint32_t instr)
{
	return instr & 0x0000003F;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint16_t
cpu_instr_imm(const uint32_t instr)
{
	return instr & UINT16_MAX;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint32_t
cpu_instr_target(const uint32_t instr)
{
	return instr & 0x03FFFFFF;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint32_t
calc_jmp_addr(const uint32_t pc, const uint32_t instr)
{
	return (cpu_instr_target(instr) << 2) + (pc & 0xF0000000);
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint32_t
calc_branch_addr(const uint32_t pc, const uint32_t instr)
{
	return sign_ext_16_32(cpu_instr_imm(instr) << 2) + pc + sizeof(instr);
}
