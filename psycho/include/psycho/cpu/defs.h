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

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define static_assert_same_word(t, m, idx) \
	_Static_assert(offsetof(t, m) / sizeof(((t *)0)->raw[0]) == (idx), #t "." #m " is not in raw[" #idx "]")

enum {
	// 33.868800 MHz
	PSYCHO_CPU_CLKFREQ_HZ = 33868800
};

enum psycho_cpu_gpr {
	PSYCHO_CPU_GPR_ZERO  = 0,
	PSYCHO_CPU_GPR_AT    = 1,
	PSYCHO_CPU_GPR_V0    = 2,
	PSYCHO_CPU_GPR_V1    = 3,
	PSYCHO_CPU_GPR_A0    = 4,
	PSYCHO_CPU_GPR_A1    = 5,
	PSYCHO_CPU_GPR_A2    = 6,
	PSYCHO_CPU_GPR_A3    = 7,
	PSYCHO_CPU_GPR_T0    = 8,
	PSYCHO_CPU_GPR_T1    = 9,
	PSYCHO_CPU_GPR_T2    = 10,
	PSYCHO_CPU_GPR_T3    = 11,
	PSYCHO_CPU_GPR_T4    = 12,
	PSYCHO_CPU_GPR_T5    = 13,
	PSYCHO_CPU_GPR_T6    = 14,
	PSYCHO_CPU_GPR_T7    = 15,
	PSYCHO_CPU_GPR_S0    = 16,
	PSYCHO_CPU_GPR_S1    = 17,
	PSYCHO_CPU_GPR_S2    = 18,
	PSYCHO_CPU_GPR_S3    = 19,
	PSYCHO_CPU_GPR_S4    = 20,
	PSYCHO_CPU_GPR_S5    = 21,
	PSYCHO_CPU_GPR_S6    = 22,
	PSYCHO_CPU_GPR_S7    = 23,
	PSYCHO_CPU_GPR_T8    = 24,
	PSYCHO_CPU_GPR_T9    = 25,
	PSYCHO_CPU_GPR_K0    = 26,
	PSYCHO_CPU_GPR_K1    = 27,
	PSYCHO_CPU_GPR_GP    = 28,
	PSYCHO_CPU_GPR_SP    = 29,
	PSYCHO_CPU_GPR_FP    = 30,
	PSYCHO_CPU_GPR_RA    = 31,
	PSYCHO_CPU_GPR_COUNT = 32
};

enum psycho_cpu_cop0_cpr {
	PSYCHO_CPU_COP0_CPR_BPC	     = 3,
	PSYCHO_CPU_COP0_CPR_BDA	     = 5,
	PSYCHO_CPU_COP0_CPR_TAR	     = 6,
	PSYCHO_CPU_COP0_CPR_DCIC     = 7,
	PSYCHO_CPU_COP0_CPR_BADVADDR = 8,
	PSYCHO_CPU_COP0_CPR_BDAM     = 9,
	PSYCHO_CPU_COP0_CPR_BPCM     = 11,
	PSYCHO_CPU_COP0_CPR_SR	     = 12,
	PSYCHO_CPU_COP0_CPR_CAUSE    = 13,
	PSYCHO_CPU_COP0_CPR_EPC	     = 14,
	PSYCHO_CPU_COP0_CPR_PRID     = 15,
	PSYCHO_CPU_COP0_CPR_COUNT    = 32
};

enum psycho_cpu_cop2_cpr {
	PSYCHO_CPU_COP2_CPR_VXY0  = 0,
	PSYCHO_CPU_COP2_CPR_VZ0	  = 1,
	PSYCHO_CPU_COP2_CPR_VXY1  = 2,
	PSYCHO_CPU_COP2_CPR_VZ1	  = 3,
	PSYCHO_CPU_COP2_CPR_VXY2  = 4,
	PSYCHO_CPU_COP2_CPR_VZ2	  = 5,
	PSYCHO_CPU_COP2_CPR_RGBC  = 6,
	PSYCHO_CPU_COP2_CPR_OTZ	  = 7,
	PSYCHO_CPU_COP2_CPR_IR0	  = 8,
	PSYCHO_CPU_COP2_CPR_IR1	  = 9,
	PSYCHO_CPU_COP2_CPR_IR2	  = 10,
	PSYCHO_CPU_COP2_CPR_IR3	  = 11,
	PSYCHO_CPU_COP2_CPR_SXY0  = 12,
	PSYCHO_CPU_COP2_CPR_SXY1  = 13,
	PSYCHO_CPU_COP2_CPR_SXY2  = 14,
	PSYCHO_CPU_COP2_CPR_SXYP  = 15,
	PSYCHO_CPU_COP2_CPR_SZ0	  = 16,
	PSYCHO_CPU_COP2_CPR_SZ1	  = 17,
	PSYCHO_CPU_COP2_CPR_SZ2	  = 18,
	PSYCHO_CPU_COP2_CPR_SZ3	  = 19,
	PSYCHO_CPU_COP2_CPR_RGB0  = 20,
	PSYCHO_CPU_COP2_CPR_RGB1  = 21,
	PSYCHO_CPU_COP2_CPR_RGB2  = 22,
	PSYCHO_CPU_COP2_CPR_RES1  = 23,
	PSYCHO_CPU_COP2_CPR_MAC0  = 24,
	PSYCHO_CPU_COP2_CPR_MAC1  = 25,
	PSYCHO_CPU_COP2_CPR_MAC2  = 26,
	PSYCHO_CPU_COP2_CPR_MAC3  = 27,
	PSYCHO_CPU_COP2_CPR_IRGB  = 28,
	PSYCHO_CPU_COP2_CPR_ORGB  = 29,
	PSYCHO_CPU_COP2_CPR_LZCS  = 30,
	PSYCHO_CPU_COP2_CPR_LZCR  = 31,
	PSYCHO_CPU_COP2_CPR_COUNT = 32
};

enum psycho_cpu_cop2_ccr {
	PSYCHO_CPU_COP2_CCR_R11R12 = 0,
	PSYCHO_CPU_COP2_CCR_R13R21 = 1,
	PSYCHO_CPU_COP2_CCR_R22R23 = 2,
	PSYCHO_CPU_COP2_CCR_R31R32 = 3,
	PSYCHO_CPU_COP2_CCR_R33    = 4,
	PSYCHO_CPU_COP2_CCR_TRX    = 5,
	PSYCHO_CPU_COP2_CCR_TRY    = 6,
	PSYCHO_CPU_COP2_CCR_TRZ    = 7,
	PSYCHO_CPU_COP2_CCR_L11L12 = 8,
	PSYCHO_CPU_COP2_CCR_L13L21 = 9,
	PSYCHO_CPU_COP2_CCR_L22L23 = 10,
	PSYCHO_CPU_COP2_CCR_L31L32 = 11,
	PSYCHO_CPU_COP2_CCR_L33    = 12,
	PSYCHO_CPU_COP2_CCR_RBK    = 13,
	PSYCHO_CPU_COP2_CCR_GBK    = 14,
	PSYCHO_CPU_COP2_CCR_BBK    = 15,
	PSYCHO_CPU_COP2_CCR_LR1LR2 = 16,
	PSYCHO_CPU_COP2_CCR_LR3LG1 = 17,
	PSYCHO_CPU_COP2_CCR_LG2LG3 = 18,
	PSYCHO_CPU_COP2_CCR_LB1LB2 = 19,
	PSYCHO_CPU_COP2_CCR_LB3    = 20,
	PSYCHO_CPU_COP2_CCR_RFC    = 21,
	PSYCHO_CPU_COP2_CCR_GFC    = 22,
	PSYCHO_CPU_COP2_CCR_BFC    = 23,
	PSYCHO_CPU_COP2_CCR_OFX    = 24,
	PSYCHO_CPU_COP2_CCR_OFY    = 25,
	PSYCHO_CPU_COP2_CCR_H      = 26,
	PSYCHO_CPU_COP2_CCR_DQA    = 27,
	PSYCHO_CPU_COP2_CCR_DQB    = 28,
	PSYCHO_CPU_COP2_CCR_ZSF3   = 29,
	PSYCHO_CPU_COP2_CCR_ZSF4   = 30,
	PSYCHO_CPU_COP2_CCR_FLAG   = 31,
	PSYCHO_CPU_COP2_CCR_COUNT  = 32
};

struct psycho_gte_sz {
	uint16_t v;
	const uint16_t pad;
};

struct psycho_gte_vec {
	union {
		struct {
			int16_t x;
			int16_t y;
			int16_t z;
			const uint16_t pad0;
		};
		int16_t arr[3];
	};
};

struct psycho_gte_sxy {
	union {
		struct {
			int16_t x;
			int16_t y;
		};
		int32_t raw;
	};
};

struct psycho_gte_rgb {
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t code;
		};
		uint32_t raw;
		uint8_t arr[4];
	};
};

struct psycho_cpu_cop2_cpr_layout {
	union {
		struct {
			struct psycho_gte_vec v[3];
			struct psycho_gte_rgb rgbc;
			uint16_t otz;
			const uint16_t pad0;
			int32_t ir[4];
			struct psycho_gte_sxy sxy[3];
			const int32_t sxyp;
			struct psycho_gte_sz sz[4];
			struct psycho_gte_rgb rgb[3];
			const uint32_t res1;
			int32_t mac[4];
			uint16_t irgb;
			const uint16_t pad1;
			uint16_t orgb;
			const uint16_t pad2;
			int32_t lzcs;
			int32_t lzcr;
		};
		uint32_t raw[PSYCHO_CPU_COP2_CPR_COUNT];
	};
};

struct psycho_cpu_cop2_ccr_layout {
	union {
		struct {
			int16_t r[3][3];
			const uint16_t pad0;
			int32_t tr[3];
			int16_t llm[3][3];
			const uint16_t pad1;
			int32_t bk[3];
			int16_t lcm[3][3];
			const uint16_t pad2;
			int32_t fc[3];
			int32_t ofx;
			int32_t ofy;
			uint16_t h;
			const uint16_t pad3;
			int16_t dqa;
			const uint16_t pad4;
			int32_t dqb;
			int16_t zsf3;
			const uint16_t pad5;
			int16_t zsf4;
			const uint16_t pad6;
			uint32_t flag;
		};
		uint32_t raw[PSYCHO_CPU_COP2_CPR_COUNT];
	};
};

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[0].x, 0);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[0].y, 0);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[0].z, 1);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[1].x, 2);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[1].y, 2);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[1].z, 3);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[2].x, 4);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[2].y, 4);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, v[2].z, 5);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, rgbc, 6);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, otz, 7);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, ir[0], 8);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, ir[1], 9);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, ir[2], 10);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, ir[3], 11);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sxy[0].x, 12);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sxy[0].y, 12);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sxy[1].x, 13);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sxy[1].y, 13);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sxy[2].x, 14);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sxy[2].y, 14);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sxyp, 15);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sz[0].v, 16);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sz[1].v, 17);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sz[2].v, 18);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, sz[3].v, 19);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, rgb[0], 20);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, rgb[1], 21);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, rgb[2], 22);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, res1, 23);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, mac[0], 24);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, mac[1], 25);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, mac[2], 26);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, mac[3], 27);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, irgb, 28);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, orgb, 29);

static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, lzcs, 30);
static_assert_same_word(struct psycho_cpu_cop2_cpr_layout, lzcr, 31);

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[0][0], 0); // RT11
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[0][1], 0); // RT12
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[0][2], 1); // RT13

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[1][0], 1); // RT21
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[1][1], 2); // RT22
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[1][2], 2); // RT23

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[2][0], 3); // RT31
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[2][1], 3); // RT32
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, r[2][2], 4); // RT33

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, tr[0], 5); // TRX
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, tr[1], 6); // TRY
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, tr[2], 7); // TRZ

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[0][0], 8); // L11
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[0][1], 8); // L12
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[0][2], 9); // L13

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[1][0], 9); // L21
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[1][1], 10); // L22
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[1][2], 10); // L23

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[2][0], 11); // L31
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[2][1], 11); // L32
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, llm[2][2], 12); // L33

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, bk[0], 13);
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, bk[1], 14);
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, bk[2], 15);

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[0][0], 16); // LR1
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[0][1], 16); // LR2
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[0][2], 17); // LR3

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[1][0], 17); // LG1
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[1][1], 18); // LG2
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[1][2], 18); // LG3

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[2][0], 19); // LB1
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[2][1], 19); // LB2
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, lcm[2][2], 20); // LB3

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, fc[0], 21);
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, fc[1], 22);
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, fc[2], 23);

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, ofx, 24);
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, ofy, 25);

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, h, 26);

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, dqa, 27);
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, dqb, 28);

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, zsf3, 29);
static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, zsf4, 30);

static_assert_same_word(struct psycho_cpu_cop2_ccr_layout, flag, 31);

#ifdef __cplusplus
}
#endif // __cplusplus
