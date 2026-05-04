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

#include "bios-trace.h"
#include "bus.h"
#include "cpu.h"
#include "disasm.h"
#include "log.h"

LOG_MODULE(PSYCHO_LOG_MODULE_CTX);

void psycho_ctx_init(struct psycho_ctx *const ctx,
		     const struct psycho_ctx_cfg *const cfg)
{
	assert(ctx != NULL);
	assert(cfg != NULL);

	psycho_log_init(ctx, &cfg->log);
	psycho_bios_trace_init(ctx, &cfg->bios_trace);
	psycho_disasm_init(ctx, &cfg->disasm);
	psycho_bus_init(ctx);
	psycho_cpu_init(ctx, &cfg->cpu);

	psycho_ctx_reset(ctx);
	LOG_INFO(ctx, "initialized");
}

void psycho_ctx_reset(struct psycho_ctx *const ctx)
{
	assert(ctx != NULL);

	psycho_cpu_reset(ctx);
	LOG_INFO(ctx, "reset");
}

void psycho_ctx_step(struct psycho_ctx *const ctx)
{
	assert(ctx != NULL);

	psycho_bios_trace_begin(ctx);
	psycho_cpu_step(ctx);
	psycho_bios_trace_end(ctx);
}
