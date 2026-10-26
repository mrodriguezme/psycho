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

#include "psycho/ctx.h"

enum {
	I_STAT = 0x1F801070,
	I_MASK = 0x1F801074,
};

#define IRQ_LIST(X)         \
	X(VBLANK, "vblank") \
	X(GPU, "gpu")       \
	X(CDROM, "cdrom")   \
	X(DMAC, "dmac")     \
	X(TMR0, "tmr0")     \
	X(TMR1, "tmr1")     \
	X(TMR2, "tmr2")     \
	X(SIO0, "sio0")     \
	X(SIO1, "sio1")     \
	X(SPU, "spu")       \
	X(LIGHTPEN, "lightpen")

enum {
#define X(id, name) IRQ_BIT_##id,
	IRQ_LIST(X)
#undef X
	// clang-format off
	IRQ_COUNT
	// clang-format on
};

enum {
#define X(id, name) IRQ_##id = 1 << IRQ_BIT_##id,
	IRQ_LIST(X)
#undef X
};

void psycho_intc_mask_set(struct psycho_ctx *ctx, uint32_t mask) PSYCHO_NONNULL;

void psycho_intc_irqs_ack(struct psycho_ctx *ctx, uint32_t mask) PSYCHO_NONNULL;

void psycho_intc_irqs_pending_set(struct psycho_ctx *ctx, uint32_t mask) PSYCHO_NONNULL;
