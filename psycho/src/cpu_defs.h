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

#define RST_VECTOR (UINT32_C(0xBFC00000))

#define SR_ISC (1 << 16)

enum instr_grp_op {
	GRP_SPECIAL = 0x00,
	GRP_REGIMM = 0x01,
	J = 0x02,
	JAL = 0x03,
	BEQ = 0x04,
	BNE = 0x05,
	BLEZ = 0x06,
	BGTZ = 0x07,
	ADDI = 0x08,
	ADDIU = 0x09,
	SLTI = 0x0A,
	SLTIU = 0x0B,
	ANDI = 0x0C,
	ORI = 0x0D,
	LUI = 0x0F,
	GRP_COP0 = 0x10,
	LB = 0x20,
	LW = 0x23,
	LBU = 0x24,
	SB = 0x28,
	SH = 0x29,
	SW = 0x2B
};

enum instr_grp_regimm {
	BLTZ = 0x00,
	BGEZ = 0x01,
};

enum instr_grp_special {
	SLL = 0x00,
	SRL = 0x02,
	SRA = 0x03,
	JR = 0x08,
	JALR = 0x09,
	MFHI = 0x10,
	MFLO = 0x12,
	DIV = 0x1A,
	DIVU = 0x1B,
	ADD = 0x20,
	ADDU = 0x21,
	SUBU = 0x23,
	AND = 0x24,
	OR = 0x25,
	SLT = 0x2A,
	SLTU = 0x2B
};

enum instr_grp_cop {
	MFC = 0x00,
	MTC = 0x04,
};

P_NODISCARD P_ALWAYS_INLINE u32 vaddr_to_paddr(const u32 vaddr)
{
	return vaddr & 0x1FFFFFFF;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_op(const u32 instr)
{
	return instr >> 26;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_rs(const u32 instr)
{
	return (instr >> 21) & 0x1F;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_rt(const u32 instr)
{
	return (instr >> 16) & 0x1F;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_rd(const u32 instr)
{
	return (instr >> 11) & 0x1F;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_shamt(const u32 instr)
{
	return (instr >> 6) & 0x1F;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_target(const u32 instr)
{
	return instr & 0x03FFFFFF;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_funct(const u32 instr)
{
	return instr & 0x0000003F;
}

P_NODISCARD P_ALWAYS_INLINE u16 instr_imm(const u32 instr)
{
	return instr & UINT16_MAX;
}

P_NODISCARD P_ALWAYS_INLINE u32 jmp_addr(const u32 pc, const u32 instr)
{
	return (instr_target(instr) << 2) + (pc & 0xF0000000);
}

P_NODISCARD P_ALWAYS_INLINE u32 branch_addr(const u32 pc, const u32 instr)
{
	return sext_16_32(instr_imm(instr) << 2) + pc + sizeof(instr);
}
