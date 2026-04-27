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

enum psycho_cpu_gpr {
	PSYCHO_CPU_REG_ZERO,
	PSYCHO_CPU_REG_AT,
	PSYCHO_CPU_REG_V0,
	PSYCHO_CPU_REG_V1,
	PSYCHO_CPU_REG_A0,
	PSYCHO_CPU_REG_A1,
	PSYCHO_CPU_REG_A2,
	PSYCHO_CPU_REG_A3,
	PSYCHO_CPU_REG_T0,
	PSYCHO_CPU_REG_T1,
	PSYCHO_CPU_REG_T2,
	PSYCHO_CPU_REG_T3,
	PSYCHO_CPU_REG_T4,
	PSYCHO_CPU_REG_T5,
	PSYCHO_CPU_REG_T6,
	PSYCHO_CPU_REG_T7,
	PSYCHO_CPU_REG_S0,
	PSYCHO_CPU_REG_S1,
	PSYCHO_CPU_REG_S2,
	PSYCHO_CPU_REG_S3,
	PSYCHO_CPU_REG_S4,
	PSYCHO_CPU_REG_S5,
	PSYCHO_CPU_REG_S6,
	PSYCHO_CPU_REG_S7,
	PSYCHO_CPU_REG_T8,
	PSYCHO_CPU_REG_T9,
	PSYCHO_CPU_REG_K0,
	PSYCHO_CPU_REG_K1,
	PSYCHO_CPU_REG_GP,
	PSYCHO_CPU_REG_SP,
	PSYCHO_CPU_REG_FP,
	PSYCHO_CPU_REG_RA,
	PSYCHO_CPU_GPR_COUNT
};

#ifdef __cplusplus
}
#endif // __cplusplus
