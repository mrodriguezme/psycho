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

#pragma once

#include "psycho/ctx.h"

enum {
	// clang-format off

	INTCTRL_ADDR_I_STAT	= 0x1F801070,
	INTCTRL_ADDR_I_MASK	= 0x1F801074,

	// clang-format on
};

enum irq {
	// clang-format off

	IRQ_VBLANK	= 1 << 0,
	IRQ_GPU		= 1 << 1,
	IRQ_CDROM	= 1 << 2,
	IRQ_DMA		= 1 << 3,
	IRQ_TMR0	= 1 << 4,
	IRQ_TMR1	= 1 << 5,
	IRQ_TMR2	= 1 << 6,
	IRQ_SIO0	= 1 << 7,
	IRQ_SIO1	= 1 << 8,
	IRQ_SPU		= 1 << 9,
	IRQ_CTRL	= 1 << 10,
	IRQ_COUNT	= 11

	// clang-format on
};

void p_irq_mask_set(struct p_ctx *ctx, const u32 mask) __attribute__((nonnull));

void p_irq_ack(struct p_ctx *ctx, const u32 mask) __attribute__((nonnull));

void p_irq_pending(struct p_ctx *ctx, const u32 mask) __attribute__((nonnull));
