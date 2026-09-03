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

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "bios_trace.h"
#include "bus.h"
#include "cpu_int.h"
#include "cpu_defs.h"
#include "gpu.h"
#include "log.h"
#include "sched.h"
#include "util.h"
#include "sio0.h"

LOG_MOD(P_LOG_CTX);

struct p_ctx_cfg *p_cfg_get(struct p_ctx *ctx)
{
	return &ctx->cfg;
}

void p_init(struct p_ctx *ctx)
{
	p_bios_trace_init(ctx);
	p_gpu_init(ctx);

	p_cpu_int_init(ctx);
	p_rst(ctx);

	LOG_INFO(ctx, "initialized");
}

void p_rst(struct p_ctx *ctx)
{
	p_sched_rst(ctx);
	p_gpu_rst(ctx);
	p_sio0_rst(ctx);

	ctx->cpu.rst(ctx);
	LOG_INFO(ctx, "reset");
}

void p_step(struct p_ctx *ctx)
{
}

void p_run_until_ev(struct p_ctx *ctx)
{
	ctx->running = true;
	ctx->cpu.run(ctx, UINT64_MAX, true);
}
