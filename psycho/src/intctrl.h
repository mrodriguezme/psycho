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

#define I_STAT	   (0x1F801070)
#define I_MASK	   (0x1F801074)

#define IRQ_VBLANK (1 << 0)
#define IRQ_GPU	   (1 << 1)
#define IRQ_CDROM  (1 << 2)
#define IRQ_DMA	   (1 << 3)
#define IRQ_TMR0   (1 << 4)
#define IRQ_TMR1   (1 << 5)
#define IRQ_TMR2   (1 << 6)
#define IRQ_SIO0   (1 << 7)
#define IRQ_SIO1   (1 << 8)
#define IRQ_SPU	   (1 << 9)
#define IRQ_CTRL   (1 << 10)
#define IRQ_COUNT  (11)

void p_irq_mask_set(struct p_ctx *ctx, u32 mask) P_NONNULL;

void p_irq_ack(struct p_ctx *ctx, u32 mask) P_NONNULL;

void p_irq_pend(struct p_ctx *ctx, u32 mask) P_NONNULL;
