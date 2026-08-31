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

#include <stddef.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define P_CPU_CLKFREQ_HZ (33868800)

enum p_cpu_gpr {
	P_ZERO,
	P_AT,
	P_V0,
	P_V1,
	P_A0,
	P_A1,
	P_A2,
	P_A3,
	P_T0,
	P_T1,
	P_T2,
	P_T3,
	P_T4,
	P_T5,
	P_T6,
	P_T7,
	P_S0,
	P_S1,
	P_S2,
	P_S3,
	P_S4,
	P_S5,
	P_S6,
	P_S7,
	P_T8,
	P_T9,
	P_K0,
	P_K1,
	P_GP,
	P_SP,
	P_FP,
	P_RA,
	P_GPR_COUNT
};

enum p_cpu_cop0 {
	P_BPC	     = 3,
	P_BDA	     = 5,
	P_TAR	     = 6,
	P_DCIC	     = 7,
	P_BADVADDR   = 8,
	P_BDAM	     = 9,
	P_BPCM	     = 11,
	P_SR	     = 12,
	P_CAUSE	     = 13,
	P_EPC	     = 14,
	P_PRID	     = 15,
	P_COP0_COUNT = 32,
};

enum p_cpu_cop2_cpr {
	P_VXY0 = 0,
	P_VZ0  = 1,
	P_VXY1 = 2,
	P_VZ1  = 3,
	P_VXY2 = 4,
	P_VZ2  = 5,
	P_RGBC = 6,
	P_OTZ  = 7,
	P_IR0  = 8,
	P_IR1  = 9,
	P_IR2  = 10,
	P_IR3  = 11,
	P_SXY0 = 12,
	P_SXY1 = 13,
	P_SXY2 = 14,
	P_SXYP = 15,
	P_SZ0  = 16,
	P_SZ1  = 17,
	P_SZ2  = 18,
	P_SZ3  = 19,
	P_RGB0 = 20,
	P_RGB1 = 21,
	P_RGB2 = 22,
	P_RES1 = 23,
	P_MAC0 = 24,
	P_MAC1 = 25,
	P_MAC2 = 26,
	P_MAC3 = 27,
	P_IRGB = 28,
	P_ORGB = 29,
	P_LZCS = 30,
	P_LZCR = 31,
	P_COP2_CPR_CNT
};

enum p_cpu_cop2_ccr {
	P_R11R12 = 0,
	P_R13R21 = 1,
	P_R22R23 = 2,
	P_R31R32 = 3,
	P_R33	 = 4,
	P_TRX	 = 5,
	P_TRY	 = 6,
	P_TRZ	 = 7,
	P_L11L12 = 8,
	P_L13L21 = 9,
	P_L22L23 = 10,
	P_L31L32 = 11,
	P_L33	 = 12,
	P_RBK	 = 13,
	P_GBK	 = 14,
	P_BBK	 = 15,
	P_LR1LR2 = 16,
	P_LR3LG1 = 17,
	P_LG2LG3 = 18,
	P_LB1LB2 = 19,
	P_LB3	 = 20,
	P_RFC	 = 21,
	P_GFC	 = 22,
	P_BFC	 = 23,
	P_OFX	 = 24,
	P_OFY	 = 25,
	P_H	 = 26,
	P_DQA	 = 27,
	P_DQB	 = 28,
	P_ZSF3	 = 29,
	P_ZSF4	 = 30,
	P_FLAG	 = 31,
	P_COP2_CCR_CNT,
};

#define static_assert_same_word(t, m, idx)                                 \
	_Static_assert(offsetof(t, m) / sizeof(((t *)0)->raw[0]) == (idx), \
		       #t "." #m " is not in raw[" #idx "]")

struct p_gte_sz {
	u16 v;
	const u16 pad;
};

struct p_gte_vec {
	union {
		struct {
			s16 x;
			s16 y;
			s16 z;
			const u16 pad0;
		};
		s16 arr[3];
	};
};

struct p_gte_sxy {
	union {
		struct {
			s16 x;
			s16 y;
		};
		s32 raw;
	};
};

struct p_gte_rgb {
	union {
		struct {
			u8 r;
			u8 g;
			u8 b;
			u8 code;
		};
		u32 raw;
		u8 arr[4];
	};
};

struct p_cop2_cpr {
	union {
		struct {
			struct p_gte_vec v[3];
			struct p_gte_rgb rgbc;
			u16 otz;
			const u16 pad0;
			s32 ir[4];
			struct p_gte_sxy sxy[3];
			const s32 sxyp;
			struct p_gte_sz sz[4];
			struct p_gte_rgb rgb[3];
			const u32 res1;
			s32 mac[4];
			u16 irgb;
			const u16 pad1;
			u16 orgb;
			const u16 pad2;
			s32 lzcs;
			s32 lzcr;
		};
		u32 raw[P_COP2_CPR_CNT];
	};
};

static_assert_same_word(struct p_cop2_cpr, v[0].x, 0);
static_assert_same_word(struct p_cop2_cpr, v[0].y, 0);
static_assert_same_word(struct p_cop2_cpr, v[0].z, 1);

static_assert_same_word(struct p_cop2_cpr, v[1].x, 2);
static_assert_same_word(struct p_cop2_cpr, v[1].y, 2);
static_assert_same_word(struct p_cop2_cpr, v[1].z, 3);

static_assert_same_word(struct p_cop2_cpr, v[2].x, 4);
static_assert_same_word(struct p_cop2_cpr, v[2].y, 4);
static_assert_same_word(struct p_cop2_cpr, v[2].z, 5);

static_assert_same_word(struct p_cop2_cpr, rgbc, 6);
static_assert_same_word(struct p_cop2_cpr, otz, 7);
static_assert_same_word(struct p_cop2_cpr, ir[0], 8);
static_assert_same_word(struct p_cop2_cpr, ir[1], 9);
static_assert_same_word(struct p_cop2_cpr, ir[2], 10);
static_assert_same_word(struct p_cop2_cpr, ir[3], 11);

static_assert_same_word(struct p_cop2_cpr, sxy[0].x, 12);
static_assert_same_word(struct p_cop2_cpr, sxy[0].y, 12);

static_assert_same_word(struct p_cop2_cpr, sxy[1].x, 13);
static_assert_same_word(struct p_cop2_cpr, sxy[1].y, 13);

static_assert_same_word(struct p_cop2_cpr, sxy[2].x, 14);
static_assert_same_word(struct p_cop2_cpr, sxy[2].y, 14);

static_assert_same_word(struct p_cop2_cpr, sxyp, 15);
static_assert_same_word(struct p_cop2_cpr, sxyp, 15);

static_assert_same_word(struct p_cop2_cpr, sz[0].v, 16);
static_assert_same_word(struct p_cop2_cpr, sz[1].v, 17);
static_assert_same_word(struct p_cop2_cpr, sz[2].v, 18);
static_assert_same_word(struct p_cop2_cpr, sz[3].v, 19);
static_assert_same_word(struct p_cop2_cpr, rgb[0], 20);
static_assert_same_word(struct p_cop2_cpr, rgb[1], 21);
static_assert_same_word(struct p_cop2_cpr, rgb[2], 22);
static_assert_same_word(struct p_cop2_cpr, res1, 23);
static_assert_same_word(struct p_cop2_cpr, mac[0], 24);
static_assert_same_word(struct p_cop2_cpr, mac[1], 25);
static_assert_same_word(struct p_cop2_cpr, mac[2], 26);
static_assert_same_word(struct p_cop2_cpr, mac[3], 27);
static_assert_same_word(struct p_cop2_cpr, irgb, 28);
static_assert_same_word(struct p_cop2_cpr, orgb, 29);
static_assert_same_word(struct p_cop2_cpr, lzcs, 30);
static_assert_same_word(struct p_cop2_cpr, lzcr, 31);

struct p_cop2_ccr {
	union {
		struct {
			s16 r[3][3];
			const u16 pad0;
			s32 tr[3];
			s16 llm[3][3];
			const u16 pad1;
			s32 bk[3];
			s16 lcm[3][3];
			const u16 pad2;
			s32 fc[3];
			s32 ofx;
			s32 ofy;
			u16 h;
			const u16 pad3;
			s16 dqa;
			const u16 pad4;
			s32 dqb;
			s16 zsf3;
			const u16 pad5;
			s16 zsf4;
			const u16 pad6;
			u32 flag;
		};
		u32 raw[P_COP2_CCR_CNT];
	};
};

static_assert_same_word(struct p_cop2_ccr, r[0][0], 0); // RT11
static_assert_same_word(struct p_cop2_ccr, r[0][1], 0); // RT12
static_assert_same_word(struct p_cop2_ccr, r[0][2], 1); // RT13

static_assert_same_word(struct p_cop2_ccr, r[1][0], 1); // RT21
static_assert_same_word(struct p_cop2_ccr, r[1][1], 2); // RT22
static_assert_same_word(struct p_cop2_ccr, r[1][2], 2); // RT23

static_assert_same_word(struct p_cop2_ccr, r[2][0], 3); // RT31
static_assert_same_word(struct p_cop2_ccr, r[2][1], 3); // RT32
static_assert_same_word(struct p_cop2_ccr, r[2][2], 4); // RT33

static_assert_same_word(struct p_cop2_ccr, tr[0], 5); // TRX
static_assert_same_word(struct p_cop2_ccr, tr[1], 6); // TRY
static_assert_same_word(struct p_cop2_ccr, tr[2], 7); // TRZ

static_assert_same_word(struct p_cop2_ccr, llm[0][0], 8); // L11
static_assert_same_word(struct p_cop2_ccr, llm[0][1], 8); // L12
static_assert_same_word(struct p_cop2_ccr, llm[0][2], 9); // L13

static_assert_same_word(struct p_cop2_ccr, llm[1][0], 9); // L21
static_assert_same_word(struct p_cop2_ccr, llm[1][1], 10); // L22
static_assert_same_word(struct p_cop2_ccr, llm[1][2], 10); // L23

static_assert_same_word(struct p_cop2_ccr, llm[2][0], 11); // L31
static_assert_same_word(struct p_cop2_ccr, llm[2][1], 11); // L32
static_assert_same_word(struct p_cop2_ccr, llm[2][2], 12); // L33

static_assert_same_word(struct p_cop2_ccr, bk[0], 13);
static_assert_same_word(struct p_cop2_ccr, bk[1], 14);
static_assert_same_word(struct p_cop2_ccr, bk[2], 15);

static_assert_same_word(struct p_cop2_ccr, lcm[0][0], 16); // LR1
static_assert_same_word(struct p_cop2_ccr, lcm[0][1], 16); // LR2
static_assert_same_word(struct p_cop2_ccr, lcm[0][2], 17); // LR3

static_assert_same_word(struct p_cop2_ccr, lcm[1][0], 17); // LG1
static_assert_same_word(struct p_cop2_ccr, lcm[1][1], 18); // LG2
static_assert_same_word(struct p_cop2_ccr, lcm[1][2], 18); // LG3

static_assert_same_word(struct p_cop2_ccr, lcm[2][0], 19); // LB1
static_assert_same_word(struct p_cop2_ccr, lcm[2][1], 19); // LB2
static_assert_same_word(struct p_cop2_ccr, lcm[2][2], 20); // LB3

static_assert_same_word(struct p_cop2_ccr, fc[0], 21);
static_assert_same_word(struct p_cop2_ccr, fc[1], 22);
static_assert_same_word(struct p_cop2_ccr, fc[2], 23);
static_assert_same_word(struct p_cop2_ccr, ofx, 24);
static_assert_same_word(struct p_cop2_ccr, ofy, 25);
static_assert_same_word(struct p_cop2_ccr, h, 26);
static_assert_same_word(struct p_cop2_ccr, dqa, 27);
static_assert_same_word(struct p_cop2_ccr, dqb, 28);
static_assert_same_word(struct p_cop2_ccr, zsf3, 29);
static_assert_same_word(struct p_cop2_ccr, zsf4, 30);
static_assert_same_word(struct p_cop2_ccr, flag, 31);

#undef static_assert_same_word

#ifdef __cplusplus
}
#endif // __cplusplus
