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
#include "bios_trace.h"
#include "bus.h"
#include "disasm.h"
#include "log.h"
#include "sched.h"

struct p_ctx_cfg {
	struct p_cpu_cfg cpu;
	struct p_bios_trace_cfg bios_trace;
	struct p_disasm_cfg disasm;
	struct p_log_cfg log;
};

struct p_ctx {
	struct p_cpu cpu;
	struct p_bios_trace bios_trace;
	struct p_bus bus;
	struct p_disasm disasm;
	struct p_sched sched;

	struct p_ctx_cfg cfg;

	struct {
		const u8 *data;
		size_t size;
	} exe;

	bool kernel_initialized;
};

enum p_ctx_ret {
	P_EXE_FILE_SIZE_INVALID = -3,
	P_EXE_SIZE_INVALID = -2,
	P_EXE_ID_INVALID = -1,
	P_OK = 1,
};

P_NODISCARD struct p_ctx_cfg *p_cfg_get(struct p_ctx *ctx)
	__attribute__((nonnull));

void p_init(struct p_ctx *ctx) __attribute__((nonnull));

void p_rst(struct p_ctx *ctx) __attribute__((nonnull));

void p_step(struct p_ctx *ctx) __attribute__((nonnull));

void p_run_until_ev(struct p_ctx *ctx) __attribute__((nonnull));

P_NODISCARD enum p_ctx_ret p_run_exe(struct p_ctx *ctx, const u8 *exe,
				     const size_t exe_size)
	__attribute__((nonnull));

#ifdef __cplusplus
}
#endif // __cplusplus
