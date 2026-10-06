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
	SIO0_TX_DATA = 0x1F801040,
	SIO0_RX_DATA = 0x1F801040,
	SIO0_STAT    = 0x1F801044,
	SIO0_MODE    = 0x1F801048,
	SIO0_CTRL    = 0x1F80104A,
	SIO0_BAUD    = 0x1F80104E
};

void psycho_sio0_reset(struct psycho_ctx *ctx) PSYCHO_NONNULL;

void psycho_sio0_tx(struct psycho_ctx *ctx, uint8_t byte) PSYCHO_NONNULL;

PSYCHO_NODISCARD uint8_t psycho_sio0_rx_pop8(struct psycho_ctx *ctx) PSYCHO_NONNULL;

void psycho_sio0_mode_set(struct psycho_ctx *ctx, uint16_t mode) PSYCHO_NONNULL;

void psycho_sio0_ctrl_set(struct psycho_ctx *ctx, uint16_t ctrl) PSYCHO_NONNULL;

void psycho_sio0_baud_set(struct psycho_ctx *ctx, uint16_t baud) PSYCHO_NONNULL;

void psycho_sio0_dsr_assert(struct psycho_ctx *ctx, void *dev) PSYCHO_NONNULL;
void psycho_sio0_dsr_deassert(struct psycho_ctx *ctx, void *dev) PSYCHO_NONNULL;
