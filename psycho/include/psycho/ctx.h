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

#include "cpu.h"
#include "bios-trace.h"
#include "bus.h"
#include "disasm.h"
#include "log.h"

struct psycho_ctx {
	struct psycho_cpu cpu;
	struct psycho_bios_trace bios_trace;
	struct psycho_bus bus;
	struct psycho_disasm disasm;
	struct psycho_log log;
};

struct psycho_ctx_cfg {
	struct psycho_cpu_cfg cpu;
	struct psycho_bios_trace_cfg bios_trace;
	struct psycho_disasm_cfg disasm;
	struct psycho_log_cfg log;
};

void psycho_ctx_init(struct psycho_ctx *ctx, const struct psycho_ctx_cfg *cfg)
	__attribute__((nonnull));

void psycho_ctx_reset(struct psycho_ctx *ctx) __attribute__((nonnull));

void psycho_ctx_step(struct psycho_ctx *ctx) __attribute__((nonnull));

#ifdef __cplusplus
}
#endif // __cplusplus
