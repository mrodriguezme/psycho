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

__attribute__((nonnull)) P_ALWAYS_INLINE void p_sched_adv_ts(struct p_ctx *ctx,
							     const u64 ts)
{
	ctx->sched.ts_now += ts;
}

void p_sched_rst(struct p_ctx *ctx) __attribute__((nonnull));

bool p_sched_run(struct p_ctx *ctx) __attribute__((nonnull));

void p_sched_add(struct p_ctx *ctx, struct p_sched_ev *ev)
	__attribute__((nonnull));

void p_sched_del(struct p_ctx *ctx, struct p_sched_ev *ev)
	__attribute__((nonnull));
