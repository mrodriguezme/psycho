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

#ifdef __cplusplus
}
#endif // __cplusplus
