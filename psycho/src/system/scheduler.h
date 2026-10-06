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

PSYCHO_STATIC_ALWAYS_INLINE void psycho_scheduler_advance_ts(struct psycho_ctx *ctx, uint64_t ts)
{
	ctx->scheduler.ts_now += ts;
}

void psycho_scheduler_reset(struct psycho_ctx *ctx) PSYCHO_NONNULL;

void psycho_scheduler_run(struct psycho_ctx *ctx) PSYCHO_NONNULL;

void psycho_scheduler_add_event(struct psycho_ctx *ctx, struct psycho_scheduler_event *ev) PSYCHO_NONNULL;
void psycho_scheduler_delete_event(struct psycho_ctx *ctx, struct psycho_scheduler_event *ev) PSYCHO_NONNULL;
