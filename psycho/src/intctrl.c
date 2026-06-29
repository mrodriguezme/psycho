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

#include "cpu.h"
#include "intctrl.h"
#include "log.h"

LOG_MOD(P_LOG_INTCTRL);

static const char *const irq_names[IRQ_COUNT] = {
	// clang-format off

	[0]	= "vblank",
	[1]	= "gpu",
	[2]	= "cdrom",
	[3]	= "dma",
	[4]	= "tmr0",
	[5]	= "tmr1",
	[6]	= "tmr2",
	[7]	= "sio0",
	[8]	= "sio1",
	[9]	= "spu",
	[10]	= "lightpen"

	// clang-format on
};

void p_irq_mask_set(struct p_ctx *const ctx, const u32 mask)
{
	u32 m_mask = mask;

	for (uint bit = 0; m_mask; m_mask >>= 1, ++bit)
		LOG_DBG(ctx, "irq \"%s\" %s", irq_names[bit],
			(m_mask & 1) ? "enabled" : "disabled");

	ctx->intctrl.i_mask = mask;
	p_cpu_irq_mux_set(ctx, (ctx->intctrl.i_mask & ctx->intctrl.i_stat));
}

void p_irq_ack(struct p_ctx *const ctx, const u32 mask)
{
	u32 m_mask = mask;

	for (uint bit = 0; m_mask; m_mask >>= 1, ++bit)
		if (((ctx->intctrl.i_stat >> bit) & 1) && !(m_mask & 1))
			LOG_DBG(ctx, "irq \"%s\" acked", irq_names[bit]);

	ctx->intctrl.i_stat &= mask;
	p_cpu_irq_mux_set(ctx, (ctx->intctrl.i_mask & ctx->intctrl.i_stat));
}

void p_irq_pending(struct p_ctx *const ctx, const u32 mask)
{
	u32 m_mask = mask;

	for (uint bit = 0; m_mask; m_mask >>= 1, ++bit)
		if (!((ctx->intctrl.i_stat >> bit) & 1) && (m_mask & 1))
			LOG_DBG(ctx, "irq \"%s\" pending", irq_names[bit]);

	ctx->intctrl.i_stat |= mask;
	p_cpu_irq_mux_set(ctx, (ctx->intctrl.i_mask & ctx->intctrl.i_stat));
}
