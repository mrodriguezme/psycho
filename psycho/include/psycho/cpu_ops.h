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

#include <stdbool.h>
#include "cpu_defs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct p_ctx;

struct p_cpu_ops {
	void (*irq_mux_set)(struct p_ctx *ctx, bool state);

	void (*gpr_set)(struct p_ctx *ctx, enum p_cpu_gpr gpr, uint32_t val);
	uint32_t (*gpr_get)(struct p_ctx *ctx, enum p_cpu_gpr gpr);

	uint32_t (*lo_get)(struct p_ctx *ctx);
	uint32_t (*hi_get)(struct p_ctx *ctx);

	void (*pc_set)(struct p_ctx *ctx, uint32_t pc);
	uint32_t (*pc_get)(struct p_ctx *ctx);

	void (*run)(struct p_ctx *ctx, uint64_t instr_limit, bool stop_on_ev);

	uint32_t (*instr_get)(struct p_ctx *ctx);

	void (*rst)(struct p_ctx *ctx);
};

#ifdef __cplusplus
}
#endif // __cplusplus
