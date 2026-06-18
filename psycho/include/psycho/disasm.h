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

#include <stdbool.h>
#include <stddef.h>

#include "compiler.h"
#include "cpu_defs.h"
#include "str.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct p_ctx;

enum {
	P_DISASM_TRACE_LEN_MAX = 512,
};

enum p_disasm_trace {
	P_DISASM_TRACE_GPR_RT,
	P_DISASM_TRACE_GPR_RD,
	P_DISASM_TRACE_CPU_LO,
	P_DISASM_TRACE_CPU_HI,
	P_DISASM_TRACE_COUNT
};

struct p_disasm_cfg {
	bool tracing;
};

struct p_disasm_traces {
	enum p_disasm_trace data[P_DISASM_TRACE_COUNT];
	size_t count;
};

struct p_disasm {
	struct {
		char str_buf[P_DISASM_TRACE_LEN_MAX];
		struct p_str str;
		u32 instr;
		u32 pc;
	} res;

	struct p_disasm_traces traces;
};

P_NODISCARD const char *p_gpr_get(const enum p_cpu_gpr reg);
P_NODISCARD const char *p_cop0_get(const enum p_cpu_cop0 reg);

void p_disasm_instr(struct p_ctx *ctx, u32 pc, struct p_disasm_traces *traces)
	__attribute__((nonnull(1)));

void p_disasm_set_tracing_state(struct p_ctx *ctx, const bool enabled)
	__attribute__((nonnull));

#ifdef __cplusplus
}
#endif // __cplusplus
