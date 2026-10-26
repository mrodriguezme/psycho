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

#include "intc.h"
#include "system/cpu/cpu.h"
#include "debug/log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_INTC);

static const char *const irq_names[IRQ_COUNT] = {
#define X(id, name) [IRQ_BIT_##id] = name,
	IRQ_LIST(X)
#undef X
};

constexpr uint32_t IRQ_ALL = (UINT32_C(1) << IRQ_COUNT) - 1;

static void irq_log_bits(struct psycho_ctx *ctx, uint32_t bits, const char *what)
{
	for (; bits; bits &= bits - 1)
		LOG_DEBUG_UNCHECKED(ctx, "irq \"%s\" %s", irq_names[__builtin_ctz(bits)], what);
}

void psycho_intc_mask_set(struct psycho_ctx *ctx, uint32_t mask)
{
	mask &= IRQ_ALL;

	if (LOG_MODULE_LEVEL_ENABLED(ctx, DEBUG)) {
		uint32_t changed = ctx->intc.i_mask ^ mask;

		irq_log_bits(ctx, changed & mask, "enabled");
		irq_log_bits(ctx, changed & ~mask, "disabled");
	}

	ctx->intc.i_mask = mask;
	psycho_cpu_irq_mux_assert(ctx, ctx->intc.i_mask & ctx->intc.i_stat);
}

void psycho_intc_irqs_ack(struct psycho_ctx *ctx, uint32_t mask)
{
	if (LOG_MODULE_LEVEL_ENABLED(ctx, DEBUG))
		irq_log_bits(ctx, ctx->intc.i_stat & ~mask & IRQ_ALL, "acked");

	ctx->intc.i_stat &= mask;
	psycho_cpu_irq_mux_assert(ctx, ctx->intc.i_mask & ctx->intc.i_stat);
}

void psycho_intc_irqs_pending_set(struct psycho_ctx *ctx, uint32_t mask)
{
	if (LOG_MODULE_LEVEL_ENABLED(ctx, DEBUG))
		irq_log_bits(ctx, mask & ~ctx->intc.i_stat & IRQ_ALL, "pending");

	ctx->intc.i_stat |= (mask & IRQ_ALL);
	psycho_cpu_irq_mux_assert(ctx, ctx->intc.i_mask & ctx->intc.i_stat);
}
