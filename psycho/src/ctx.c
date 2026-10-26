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

#include "system/sched.h"
#include "common/util.h"

#include "debug/bios_call_tracer.h"
#include "debug/log.h"

#include "system/cpu/cpu.h"
#include "system/cpu/backend/interpreter.h"
#include "system/gpu/backend/software/software.h"
#include "system/gpu/gpu.h"
#include "system/sio0/sio0.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_CTX);

static const struct psycho_cpu_backend_ops *const cpu_backends[PSYCHO_CPU_BACKEND_COUNT] = {
	[PSYCHO_CPU_BACKEND_INTERPRETER] = &psycho_cpu_backend_ops_interpreter,
};

static const struct psycho_gpu_backend_ops *const gpu_backends[PSYCHO_GPU_BACKEND_COUNT] = {
	[PSYCHO_GPU_BACKEND_SOFTWARE] = &psycho_gpu_backend_ops_software,
};

static int cpu_backend_set(struct psycho_ctx *ctx)
{
	enum psycho_cpu_backend b = ctx->cfg.cpu.backend;

	if ((b >= ARRAY_SIZE(cpu_backends)) || (!cpu_backends[b]))
		return -1;

	const struct psycho_cpu_backend_ops *next = cpu_backends[b];
	const struct psycho_cpu_backend_ops *prev = ctx->cpu.backend;

	if (unlikely(next == prev))
		return PSYCHO_CTX_OK;

	if ((next->init) && (!next->init(ctx)))
		return PSYCHO_CTX_CPU_BACKEND_ERROR;

	if ((prev) && (prev->fini))
		prev->fini(ctx);

	psycho_cpu_reset(ctx);
	ctx->cpu.backend = next;

	LOG_INFO(ctx, "cpu backend set to \"%s\"", ctx->cpu.backend->name);
	return PSYCHO_CTX_OK;
}

static int gpu_backend_set(struct psycho_ctx *ctx)
{
	enum psycho_gpu_backend b = ctx->cfg.gpu.backend;

	if ((b >= ARRAY_SIZE(gpu_backends)) || (!gpu_backends[b]))
		return -1;

	const struct psycho_gpu_backend_ops *next = gpu_backends[b];
	const struct psycho_gpu_backend_ops *prev = ctx->gpu.backend;

	if (unlikely(next == prev))
		return PSYCHO_CTX_OK;

	if ((next->init) && (!next->init(ctx)))
		return PSYCHO_CTX_CPU_BACKEND_ERROR;

	if ((prev) && (prev->fini))
		prev->fini(ctx);

	psycho_gpu_reset(ctx);
	ctx->gpu.backend = next;

	LOG_INFO(ctx, "gpu backend set to \"%s\"", ctx->gpu.backend->name);
	return PSYCHO_CTX_OK;
}


struct psycho_ctx_cfg *psycho_ctx_cfg_get(struct psycho_ctx *ctx)
{
	return &ctx->cfg;
}

enum psycho_ctx_ret psycho_ctx_init(struct psycho_ctx *ctx)
{
	enum psycho_ctx_ret ret;

	if ((ret = cpu_backend_set(ctx)) != PSYCHO_CTX_OK)
		return ret;

	if ((ret = gpu_backend_set(ctx)) != PSYCHO_CTX_OK)
		return ret;

	psycho_bios_call_tracer_init(ctx);
	psycho_ctx_reset(ctx);

	LOG_INFO(ctx, "initialized");
	return ret;
}

void psycho_ctx_reset(struct psycho_ctx *ctx)
{
	psycho_sched_reset(ctx);
	psycho_sio0_reset(ctx);
	psycho_gpu_reset(ctx);

	ctx->gpu.backend->reset(ctx);
	ctx->cpu.backend->reset(ctx);

	LOG_INFO(ctx, "reset");
}

void psycho_ctx_run(struct psycho_ctx *ctx)
{
	ctx->running = true;
	ctx->cpu.backend->run(ctx, false);
}

void psycho_ctx_stop(struct psycho_ctx *ctx)
{
	ctx->running = false;
}
