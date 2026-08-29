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

#define RST_VECTOR   (UINT32_C(0xBFC00000))

#define SR_ISC	     (1 << 16)

#define FLAG_ERR     (UINT32_C(1) << 31)
#define MAC1_OVF_POS (1 << 30)
#define MAC2_OVF_POS (1 << 29)
#define MAC3_OVF_POS (1 << 28)
#define MAC1_OVF_NEG (1 << 27)
#define MAC2_OVF_NEG (1 << 26)
#define MAC3_OVF_NEG (1 << 25)
#define IR1_SAT	     (1 << 24)
#define IR2_SAT	     (1 << 23)
#define IR3_SAT	     (1 << 22)
#define RGB_R_SAT    (1 << 21)
#define RGB_G_SAT    (1 << 20)
#define RGB_B_SAT    (1 << 19)
#define SZ3_OTZ_SAT  (1 << 18)
#define DIV_OVF	     (1 << 17)
#define MAC0_POS_OVF (1 << 16)
#define MAC0_NEG_OVF (1 << 15)
#define SX2_SAT	     (1 << 14)
#define SY2_SAT	     (1 << 13)
#define IR0_SAT	     (1 << 12)

#define FLAG_MASK                                                    \
	(MAC1_OVF_POS | MAC2_OVF_POS | MAC3_OVF_POS | MAC1_OVF_NEG | \
	 MAC2_OVF_NEG | MAC3_OVF_NEG | IR1_SAT | IR2_SAT | IR3_SAT | \
	 RGB_R_SAT | RGB_G_SAT | RGB_B_SAT | SZ3_OTZ_SAT | DIV_OVF | \
	 MAC0_POS_OVF | MAC0_NEG_OVF | SX2_SAT | SY2_SAT | IR0_SAT)

#define FLAG_ERR_MASK                                                    \
	(MAC1_OVF_POS | MAC2_OVF_POS | MAC3_OVF_POS | MAC1_OVF_NEG |     \
	 MAC2_OVF_NEG | MAC3_OVF_NEG | IR1_SAT | IR2_SAT | SZ3_OTZ_SAT | \
	 DIV_OVF | MAC0_POS_OVF | MAC0_NEG_OVF | SX2_SAT | SY2_SAT)

#define MAC123_MAX	 ((INT64_C(1) << 43) - 1)
#define MAC123_MIN	 (-(INT64_C(1) << 43))

#define IR0_MIN		 (0x0000)
#define IR0_MAX		 (0x1000)

#define IR123_MIN	 (-(INT16_C(1) << 15))
#define IR123_MAX	 ((INT16_C(1) << 15) - 1)
#define IR123_LM_MIN	 (0x0000)

#define SXY_MIN		 (-0x0400)
#define SXY_MAX		 (+0x03FF)

#define SZ_OTZ_MIN	 (0x0000)
#define SZ_OTZ_MAX	 (UINT16_MAX)

#define RGB_MIN		 (0x00)
#define RGB_MAX		 (UINT8_MAX)

#define MVMVA_MX_SHIFT	 (17)
#define MVMVA_VX_SHIFT	 (15)
#define MVMVA_TX_SHIFT	 (13)

#define MVMVA_PARAM_MASK (0x3)

#define INSTR_SF	 (1 << 19)
#define INSTR_LM	 (1 << 10)

enum cpu_exc {
	EXC_ADEL    = 4,
	EXC_ADES    = 5,
	EXC_SYSCALL = 8,
	EXC_BP	    = 9,
	EXC_OV	    = 12,
};

enum instr_grp_op {
	GRP_SPECIAL = 0x00,
	GRP_REGIMM  = 0x01,
	J	    = 0x02,
	JAL	    = 0x03,
	BEQ	    = 0x04,
	BNE	    = 0x05,
	BLEZ	    = 0x06,
	BGTZ	    = 0x07,
	ADDI	    = 0x08,
	ADDIU	    = 0x09,
	SLTI	    = 0x0A,
	SLTIU	    = 0x0B,
	ANDI	    = 0x0C,
	ORI	    = 0x0D,
	XORI	    = 0x0E,
	LUI	    = 0x0F,
	GRP_COP0    = 0x10,
	GRP_COP2    = 0x12,
	LB	    = 0x20,
	LH	    = 0x21,
	LWL	    = 0x22,
	LW	    = 0x23,
	LBU	    = 0x24,
	LHU	    = 0x25,
	LWR	    = 0x26,
	SB	    = 0x28,
	SH	    = 0x29,
	SWL	    = 0x2A,
	SW	    = 0x2B,
	SWR	    = 0x2E,
};

enum instr_grp_special {
	SLL	= 0x00,
	SRL	= 0x02,
	SRA	= 0x03,
	SLLV	= 0x04,
	SRLV	= 0x06,
	SRAV	= 0x07,
	JR	= 0x08,
	JALR	= 0x09,
	SYSCALL = 0x0C,
	BREAK	= 0x0D,
	MFHI	= 0x10,
	MTHI	= 0x11,
	MFLO	= 0x12,
	MTLO	= 0x13,
	MULT	= 0x18,
	MULTU	= 0x19,
	DIV	= 0x1A,
	DIVU	= 0x1B,
	ADD	= 0x20,
	ADDU	= 0x21,
	SUB	= 0x22,
	SUBU	= 0x23,
	AND	= 0x24,
	OR	= 0x25,
	XOR	= 0x26,
	NOR	= 0x27,
	SLT	= 0x2A,
	SLTU	= 0x2B
};

enum instr_grp_cop {
	MFC = 0x00,
	CFC = 0x02,
	MTC = 0x04,
	CTC = 0x06,
};

enum instr_grp_cop0 {
	RFE = 0x10,
};

enum instr_grp_cop2 {
	RTPS  = 0x01,
	NCLIP = 0x06,
	OP    = 0x0C,
	DPCS  = 0x10,
	INTPL = 0x11,
	MVMVA = 0x12,
	NCDS  = 0x13,
	CDP   = 0x14,
	NCDT  = 0x16,
	NCCS  = 0x1B,
	CC    = 0x1C,
	NCS   = 0x1E,
	NCT   = 0x20,
	SQR   = 0x28,
	DPCL  = 0x29,
	DPCT  = 0x2A,
	AVSZ3 = 0x2D,
	AVSZ4 = 0x2E,
	RTPT  = 0x30,
	GPF   = 0x3D,
	GPL   = 0x3E,
	NCCT  = 0x3F
};

P_NODISCARD P_ALWAYS_INLINE uint mvmva_mx(u32 instr)
{
	return (instr >> MVMVA_MX_SHIFT) & MVMVA_PARAM_MASK;
}

P_NODISCARD P_ALWAYS_INLINE uint mvmva_vx(u32 instr)
{
	return (instr >> MVMVA_VX_SHIFT) & MVMVA_PARAM_MASK;
}

P_NODISCARD P_ALWAYS_INLINE uint mvmva_tx(u32 instr)
{
	return (instr >> MVMVA_TX_SHIFT) & MVMVA_PARAM_MASK;
}

P_NODISCARD P_ALWAYS_INLINE uint shift_frac(u32 instr)
{
	return (instr & INSTR_SF) ? 12 : 0;
}

P_NODISCARD P_ALWAYS_INLINE bool ir123_lm(u32 instr)
{
	return instr & INSTR_LM;
}

P_NODISCARD P_ALWAYS_INLINE u32 vaddr_to_paddr(u32 vaddr)
{
	return vaddr & 0x1FFFFFFF;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_op(u32 instr)
{
	return instr >> 26;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_rs(u32 instr)
{
	return (instr >> 21) & 0x1F;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_rt(u32 instr)
{
	return (instr >> 16) & 0x1F;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_rd(u32 instr)
{
	return (instr >> 11) & 0x1F;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_shamt(u32 instr)
{
	return (instr >> 6) & 0x1F;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_target(u32 instr)
{
	return instr & 0x03FFFFFF;
}

P_NODISCARD P_ALWAYS_INLINE uint instr_funct(u32 instr)
{
	return instr & 0x0000003F;
}

P_NODISCARD P_ALWAYS_INLINE u16 instr_imm(u32 instr)
{
	return instr & UINT16_MAX;
}

P_NODISCARD P_ALWAYS_INLINE u32 jmp_addr(u32 pc, u32 instr)
{
	return (instr_target(instr) << 2) + (pc & 0xF0000000);
}

P_NODISCARD P_ALWAYS_INLINE u32 branch_addr(u32 pc, u32 instr)
{
	return sext_16_32(instr_imm(instr) << 2) + pc + sizeof(instr);
}
