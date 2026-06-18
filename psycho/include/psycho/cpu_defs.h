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
	P_BPC = 3,
	P_BDA = 5,
	P_TAR = 6,
	P_DCIC = 7,
	P_BADVADDR = 8,
	P_BDAM = 9,
	P_BPCM = 11,
	P_SR = 12,
	P_CAUSE = 13,
	P_EPC = 14,
	P_PRID = 15,
	P_COP0_COUNT = 32,
};

#ifdef __cplusplus
}
#endif // __cplusplus
