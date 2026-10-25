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

#pragma once

#include <assert.h>
#include "common/util.h"

// ---------------------------------------------------------------------------
// Instruction/opcode bit fields
// ---------------------------------------------------------------------------
constexpr uint32_t INSTR_OP	   = GENMASK32(31, 26);
constexpr uint32_t INSTR_RS	   = GENMASK32(25, 21);
constexpr uint32_t INSTR_RT	   = GENMASK32(20, 16);
constexpr uint32_t INSTR_RD	   = GENMASK32(15, 11);
constexpr uint32_t INSTR_SHAMT	   = GENMASK32(10, 6);
constexpr uint32_t INSTR_FUNCT	   = GENMASK32(5, 0);
constexpr uint32_t INSTR_TARGET	   = GENMASK32(25, 0);
constexpr uint32_t INSTR_IMMEDIATE = GENMASK32(15, 0);

constexpr uint32_t INSTR_SF	  = 1 << 19;
constexpr uint32_t INSTR_MVMVA_MX = GENMASK32(18, 17);
constexpr uint32_t INSTR_MVMVA_VX = GENMASK32(16, 15);
constexpr uint32_t INSTR_MVMVA_TX = GENMASK32(14, 13);
constexpr uint32_t INSTR_LM	  = 1 << 10;

// ---------------------------------------------------------------------------
// Exception codes
// ---------------------------------------------------------------------------

// clang-format off

#define EXCEPTIONS(X)										\
	X(EXCEPTION_INT,	0,	"Int",		"external interrupt")			\
	X(EXCEPTION_ADEL,	4,	"AdEL",		"address error on load/fetch")		\
	X(EXCEPTION_ADES,	5,	"AdES",		"address error on store")		\
	X(EXCEPTION_IBE,	6,	"IBE",		"bus error on instruction fetch")	\
	X(EXCEPTION_DBE,	7,	"DBE",		"bus error on data load/store")		\
	X(EXCEPTION_SYSCALL,	8,	"Syscall",	"syscall instruction")			\
	X(EXCEPTION_BP,		9,	"BP",		"breakpoint instruction")		\
	X(EXCEPTION_RI,		10,	"RI",		"reserved instruction")			\
	X(EXCEPTION_CPU,	11,	"CpU",		"coprocessor unusable")			\
	X(EXCEPTION_OV,		12,	"Ov",		"arithmetic overflow")

// clang-format on

enum cpu_exception {
#define X(name, val, mnemonic, desc) name = val,
	EXCEPTIONS(X)
#undef X
};

struct cpu_exception_info {
	const char *mnemonic;
	const char *desc;
};

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE const struct cpu_exception_info *exception_info(unsigned int code)
{
	static const struct cpu_exception_info infos[32] = {
#define X(name, val, mnemonic, desc) [name] = { mnemonic, desc },
		EXCEPTIONS(X)
#undef X
	};

	assert((code < ARRAY_SIZE(infos)) && (infos[code].mnemonic));
	return &infos[code];
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE const char *exception_mnemonic(unsigned int code)
{
	return exception_info(code)->mnemonic;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE const char *exception_desc(unsigned int code)
{
	return exception_info(code)->desc;
}

// ---------------------------------------------------------------------------
// Exception vectors
// ---------------------------------------------------------------------------
enum {
	VECTOR_DEBUG	 = 0x80000040,
	VECTOR_EXCEPTION = 0x80000080,
	VECTOR_RESET	 = 0xBFC00000,
};

// ---------------------------------------------------------------------------
// "DCIC - Debug and Cache Invalidate Control" register layout
// ---------------------------------------------------------------------------
constexpr uint32_t DCIC_TR  = UINT32_C(1) << 31;
constexpr uint32_t DCIC_UD  = 1 << 30;
constexpr uint32_t DCIC_KD  = 1 << 29;
constexpr uint32_t DCIC_TE  = 1 << 28;
constexpr uint32_t DCIC_DW  = 1 << 27;
constexpr uint32_t DCIC_DR  = 1 << 26;
constexpr uint32_t DCIC_DAE = 1 << 25;
constexpr uint32_t DCIC_PCE = 1 << 24;
constexpr uint32_t DCIC_DE  = 1 << 23;
constexpr uint32_t DCIC_T   = 1 << 5;
constexpr uint32_t DCIC_W   = 1 << 4;
constexpr uint32_t DCIC_R   = 1 << 3;
constexpr uint32_t DCIC_DA  = 1 << 2;
constexpr uint32_t DCIC_PC  = 1 << 1;
constexpr uint32_t DCIC_DB  = 1 << 0;

constexpr uint32_t DCIC_BP_WRITE_EN_MASK = DCIC_TR | DCIC_UD | DCIC_DW | DCIC_DAE | DCIC_DE;

// ---------------------------------------------------------------------------
// "SR - Status Register" register layout
// ---------------------------------------------------------------------------
constexpr uint32_t SR_CU3     = UINT32_C(1) << 31;
constexpr uint32_t SR_CU2     = 1 << 30;
constexpr uint32_t SR_CU1     = 1 << 29;
constexpr uint32_t SR_CU0     = 1 << 28;
constexpr uint32_t SR_RE      = 1 << 25;
constexpr uint32_t SR_BEV     = 1 << 22;
constexpr uint32_t SR_TS      = 1 << 21;
constexpr uint32_t SR_PE      = 1 << 20;
constexpr uint32_t SR_CM      = 1 << 19;
constexpr uint32_t SR_PZ      = 1 << 18;
constexpr uint32_t SR_SWC     = 1 << 17;
constexpr uint32_t SR_ISC     = 1 << 16;
constexpr uint32_t SR_IM_MASK = GENMASK32(15, 8);
constexpr uint32_t SR_IM7     = 1 << 15;
constexpr uint32_t SR_IM6     = 1 << 14;
constexpr uint32_t SR_IM5     = 1 << 13;
constexpr uint32_t SR_IM4     = 1 << 12;
constexpr uint32_t SR_IM3     = 1 << 11;
constexpr uint32_t SR_IM2     = 1 << 10;
constexpr uint32_t SR_IM1     = 1 << 9;
constexpr uint32_t SR_IM0     = 1 << 8;
constexpr uint32_t SR_KUO     = 1 << 5;
constexpr uint32_t SR_IEO     = 1 << 4;
constexpr uint32_t SR_KUP     = 1 << 3;
constexpr uint32_t SR_IEP     = 1 << 2;
constexpr uint32_t SR_KUC     = 1 << 1;
constexpr uint32_t SR_IEC     = 1 << 0;

// ---------------------------------------------------------------------------
// Geometry Transformation Engine (GTE/COP2) "FLAG" control register layout
// ---------------------------------------------------------------------------
constexpr uint32_t FLAG_ERR	     = UINT32_C(1) << 31;
constexpr uint32_t MAC1_OVERFLOW_POS = 1 << 30;
constexpr uint32_t MAC2_OVERFLOW_POS = 1 << 29;
constexpr uint32_t MAC3_OVERFLOW_POS = 1 << 28;
constexpr uint32_t MAC1_OVERFLOW_NEG = 1 << 27;
constexpr uint32_t MAC2_OVERFLOW_NEG = 1 << 26;
constexpr uint32_t MAC3_OVERFLOW_NEG = 1 << 25;
constexpr uint32_t IR1_SATURATED     = 1 << 24;
constexpr uint32_t IR2_SATURATED     = 1 << 23;
constexpr uint32_t IR3_SATURATED     = 1 << 22;
constexpr uint32_t RGB_R_SATURATED   = 1 << 21;
constexpr uint32_t RGB_G_SATURATED   = 1 << 20;
constexpr uint32_t RGB_B_SATURATED   = 1 << 19;
constexpr uint32_t SZ3_OTZ_SATURATED = 1 << 18;
constexpr uint32_t DIV_OVERFLOW	     = 1 << 17;
constexpr uint32_t MAC0_OVERFLOW_POS = 1 << 16;
constexpr uint32_t MAC0_OVERFLOW_NEG = 1 << 15;
constexpr uint32_t SX2_SATURATED     = 1 << 14;
constexpr uint32_t SY2_SATURATED     = 1 << 13;
constexpr uint32_t IR0_SATURATED     = 1 << 12;

constexpr uint32_t FLAG_MASK = MAC1_OVERFLOW_POS | MAC2_OVERFLOW_POS | MAC3_OVERFLOW_POS | MAC1_OVERFLOW_NEG |
			       MAC2_OVERFLOW_NEG | MAC3_OVERFLOW_NEG | IR1_SATURATED | IR2_SATURATED | IR3_SATURATED |
			       RGB_R_SATURATED | RGB_G_SATURATED | RGB_B_SATURATED | SZ3_OTZ_SATURATED | DIV_OVERFLOW |
			       MAC0_OVERFLOW_POS | MAC0_OVERFLOW_NEG | SX2_SATURATED | SY2_SATURATED | IR0_SATURATED;

constexpr uint32_t FLAG_ERR_MASK = MAC1_OVERFLOW_POS | MAC2_OVERFLOW_POS | MAC3_OVERFLOW_POS | MAC1_OVERFLOW_NEG |
				   MAC2_OVERFLOW_NEG | MAC3_OVERFLOW_NEG | IR1_SATURATED | IR2_SATURATED |
				   SZ3_OTZ_SATURATED | DIV_OVERFLOW | MAC0_OVERFLOW_POS | MAC0_OVERFLOW_NEG |
				   SX2_SATURATED | SY2_SATURATED;

// ---------------------------------------------------------------------------
// Geometry Transformation Engine (GTE/COP2) fixed-point arithmetic
// ---------------------------------------------------------------------------
constexpr unsigned int SHIFT_FRAC = 12;

// ---------------------------------------------------------------------------
// Geometry Transformation Engine (GTE/COP2) register limits
// ---------------------------------------------------------------------------
constexpr int64_t MAC123_MIN = -(INT64_C(1) << 43);
constexpr int64_t MAC123_MAX = (INT64_C(1) << 43) - 1;

constexpr int32_t SXY_MIN = -0x0400;
constexpr int32_t SXY_MAX = 0x03FF;

constexpr int16_t IR0_MIN      = 0x0000;
constexpr int16_t IR0_MAX      = 0x1000;
constexpr int16_t IR123_MIN    = INT16_MIN;
constexpr int16_t IR123_MAX    = INT16_MAX;
constexpr int16_t IR123_LM_MIN = 0x0000;

constexpr uint16_t SZ_OTZ_MIN = 0x0000;
constexpr uint16_t SZ_OTZ_MAX = UINT16_MAX;

constexpr uint8_t RGB_MIN = 0x00;
constexpr uint8_t RGB_MAX = UINT8_MAX;

// ---------------------------------------------------------------------------
// OP group instructions
// ---------------------------------------------------------------------------
enum {
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

// ---------------------------------------------------------------------------
// SPECIAL group instructions
// ---------------------------------------------------------------------------
enum {
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

// ---------------------------------------------------------------------------
// COP group instructions
// ---------------------------------------------------------------------------
enum {
	MFC = 0x00,
	CFC = 0x02,
	MTC = 0x04,
	CTC = 0x06,
};

// ---------------------------------------------------------------------------
// COP0 group instructions
// ---------------------------------------------------------------------------
enum {
	RFE = 0x10,
};

// ---------------------------------------------------------------------------
// Geometry Transformation Engine (GTE/COP2) instructions
// ---------------------------------------------------------------------------
enum {
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

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint32_t jmp_addr(uint32_t pc, uint32_t instr)
{
	return (field_get32(INSTR_TARGET, instr) << 2) + (pc & 0xF0000000);
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint32_t branch_addr(uint32_t pc, uint32_t instr)
{
	return sign_extend_16_32(field_get16(INSTR_IMMEDIATE, instr) << 2) + pc + sizeof(instr);
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE unsigned int shift_frac(uint32_t instr)
{
	return (instr & INSTR_SF) ? SHIFT_FRAC : 0;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE bool ir123_lm(uint32_t instr)
{
	return instr & INSTR_LM;
}

PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint32_t vaddr_to_paddr(uint32_t vaddr)
{
	return ((vaddr >> 30) == 2) ? (vaddr & 0x1FFFFFFF) : vaddr;
}

extern const uint8_t psycho_gte_unr_tbl[0x101];
