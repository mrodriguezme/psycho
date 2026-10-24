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

#include "common/util.h"

#include "system/sched.h"

#include "debug/bios_call_tracer.h"
#include "debug/log.h"

#include "system/cpu/backend/interpreter.h"
#include "system/gpu/backend/software/software.h"
#include "system/sio0/sio0.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_CTX);

static const struct psycho_cpu_backend_ops *const cpu_backends[PSYCHO_CPU_BACKEND_COUNT] = {
	[PSYCHO_CPU_BACKEND_INTERPRETER] = &psycho_cpu_backend_ops_interpreter,
};

static const struct psycho_gpu_backend_ops *const gpu_backends[PSYCHO_GPU_BACKEND_COUNT] = {
	[PSYCHO_GPU_BACKEND_SOFTWARE] = &psycho_gpu_backend_ops_software,
};

static int cpu_backend_apply(struct psycho_ctx *ctx)
{
	enum psycho_cpu_backend b = ctx->cfg.cpu.backend;

	if ((b >= ARRAY_SIZE(cpu_backends)) || (!cpu_backends[b]))
		return -1;

	const struct psycho_cpu_backend_ops *next = cpu_backends[b];
	const struct psycho_cpu_backend_ops *prev = &ctx->cpu.ops;

	if (next == prev)
		return 0;

	if (next->init && next->init(ctx) != 0)
		return -1;

	if (prev && prev->shutdown)
		prev->shutdown(ctx);

	ctx->cpu.ops = *next;
	return 0;
}

struct psycho_ctx_cfg *psycho_ctx_cfg_get(struct psycho_ctx *ctx)
{
	return &ctx->cfg;
}

void psycho_ctx_init(struct psycho_ctx *ctx)
{
	psycho_bios_call_tracer_init(ctx);
	psycho_gpu_init(ctx);
	psycho_ctx_reset(ctx);

	LOG_INFO(ctx, "initialized");
}

void psycho_ctx_reset(struct psycho_ctx *ctx)
{
	psycho_sched_reset(ctx);
	psycho_gpu_reset(ctx);
	psycho_sio0_reset(ctx);

	ctx->cpu.reset(ctx);
	LOG_INFO(ctx, "reset");
}

void psycho_ctx_run(struct psycho_ctx *ctx)
{
	ctx->running = true;
	ctx->cpu.ops.run(ctx);
}

void psycho_ctx_pause(struct psycho_ctx *ctx)
{
	ctx->running = false;
}
