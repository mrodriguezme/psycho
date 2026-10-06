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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "system/cpu/interpreter.h"
#include "system/cpu/ops.h"
#include "system/bus.h"
#include "system/dma.h"
#include "system/gpu.h"
#include "system/interrupt_controller.h"
#include "system/sio0.h"

#include "debug/bios_func_tracer.h"
#include "debug/disasm.h"
#include "debug/log.h"

#include "scheduler.h"

struct psycho_ctx_cfg {
	struct psycho_cpu_cfg cpu;
	struct psycho_bios_func_tracer_cfg bios_func_tracer;
	struct psycho_disasm_cfg disasm;
	struct psycho_log_cfg log;

	void (*on_vblank)(struct psycho_ctx *ctx);
};

struct psycho_ctx {
	struct psycho_bus bus;

	struct {
		struct psycho_cpu_interpreter interpreter;
		struct psycho_cpu_ops impl;
	} cpu;

	struct psycho_bios_func_tracer bios_func_tracer;
	struct psycho_disasm disasm;
	struct psycho_scheduler scheduler;
	struct psycho_gpu gpu;
	struct psycho_interrupt_controller interrupt_controller;
	struct psycho_sio0 sio0;

	struct psycho_ctx_cfg cfg;

	struct {
		uint8_t *data;
		size_t size;
	} exe;

	bool running;
};

enum psycho_ctx_ret {
	PSYCHO_EXE_FILE_SIZE_INVALID = -3,
	PSYCHO_EXE_SIZE_INVALID	     = -2,
	PSYCHO_EXE_ID_INVALID	     = -1,
	PSYCHO_OK		     = 1,
};

PSYCHO_NODISCARD PSYCHO_CONST struct psycho_ctx_cfg *psycho_ctx_cfg_get(struct psycho_ctx *ctx) PSYCHO_NONNULL;

void psycho_ctx_init(struct psycho_ctx *ctx) PSYCHO_NONNULL;

void psycho_ctx_reset(struct psycho_ctx *ctx) PSYCHO_NONNULL;

void psycho_ctx_step(struct psycho_ctx *ctx) PSYCHO_NONNULL;

PSYCHO_NODISCARD enum psycho_ctx_ret psycho_ctx_run_exe(struct psycho_ctx *ctx, uint8_t *exe,
							size_t size) PSYCHO_NONNULL;

void psycho_ctx_run_until_event(struct psycho_ctx *ctx) PSYCHO_NONNULL;

#ifdef __cplusplus
}
#endif // __cplusplus
