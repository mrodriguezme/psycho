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
#include "debug/log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_INTC);

static const char *irq_names[IRQ_COUNT] = { [0] = "vblank", [1] = "gpu",  [2] = "cdrom",    [3] = "dma",
					    [4] = "tmr0",   [5] = "tmr1", [6] = "tmr2",	    [7] = "sio0",
					    [8] = "sio1",   [9] = "spu",  [10] = "lightpen" };

static void irq_mask_log(struct p_ctx *ctx, uint32_t mask)
{
	for (unsigned int bit = 0; mask; mask >>= 1, ++bit)
		LOG_DEBUG_UNCHECKED(ctx, "irq \"%s\" %s", irq_names[bit], (mask & 1) ? "enabled" : "disabled");
}

static void irq_ack_log(struct p_ctx *ctx, uint32_t mask)
{
	for (unsigned int bit = 0; mask; mask >>= 1, ++bit)
		if (((ctx->intc.i_stat >> bit) & 1) && !(mask & 1))
			LOG_DEBUG_UNCHECKED(ctx, "irq \"%s\" acked", irq_names[bit]);
}

static void irq_pending_log(struct p_ctx *ctx, uint32_t mask)
{
	for (unsigned int bit = 0; mask; mask >>= 1, ++bit)
		if (!((ctx->intc.i_stat >> bit) & 1) && (mask & 1))
			LOG_DEBUG_UNCHECKED(ctx, "irq \"%s\" pending", irq_names[bit]);
}

void psycho_intc_mask_set(struct p_ctx *ctx, uint32_t mask)
{
	if (LOG_MODULE_LEVEL_ENABLED(ctx, DEBUG))
		irq_mask_log(ctx, mask);

	ctx->intc.i_mask = mask;
	ctx->cpu.irq_mux_set(ctx, ctx->intc.i_mask & ctx->intc.i_stat);
}

void psycho_intc_irqs_ack(struct p_ctx *ctx, uint32_t mask)
{
	if (LOG_MODULE_LEVEL_ENABLED(ctx, DEBUG))
		irq_ack_log(ctx, mask);

	ctx->intc.i_stat &= mask;
	ctx->cpu.irq_mux_set(ctx, ctx->intc.i_mask & ctx->intc.i_stat);
}

void psycho_intc_irqs_pending_set(struct p_ctx *ctx, uint32_t mask)
{
	if (LOG_MODULE_LEVEL_ENABLED(ctx, DEBUG))
		irq_pending_log(ctx, mask);

	ctx->intc.i_stat |= mask;
	ctx->cpu.irq_mux_set(ctx, ctx->intc.i_mask & ctx->intc.i_stat);
}
