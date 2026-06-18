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

#include "bios_trace.h"
#include "bus.h"
#include "cpu.h"
#include "log.h"

LOG_MOD(P_LOG_CTX);

struct p_ctx_cfg *p_ctx_cfg_get(struct p_ctx *const ctx)
{
	return &ctx->cfg;
}

void p_ctx_init(struct p_ctx *const ctx)
{
	p_bios_trace_init(ctx);
	p_bus_init(ctx);

	p_ctx_rst(ctx);
	LOG_INFO(ctx, "initialized");
}

void p_ctx_rst(struct p_ctx *const ctx)
{
	p_cpu_rst(ctx);
	LOG_INFO(ctx, "reset");
}

void p_ctx_step(struct p_ctx *const ctx)
{
	p_bios_trace_begin(ctx);
	p_cpu_step(ctx);
	p_bios_trace_end(ctx);
}
