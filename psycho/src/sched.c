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
#include <string.h>

#include "sched.h"
#include "log.h"
#include "util.h"

LOG_MOD(P_LOG_SCHED);

P_NODISCARD static size_t node_parent(const size_t node)
{
	return (node - 1) / 2;
}

P_NODISCARD static size_t node_left(const size_t node)
{
	return (node * 2) + 1;
}

P_NODISCARD static size_t node_right(const size_t node)
{
	return node_left(node) + 1;
}

__attribute__((nonnull)) static void heap_swap(struct p_ctx *const ctx,
					       const size_t a, const size_t b)
{
	swap(ctx->sched.ev[a], ctx->sched.ev[b]);
	ctx->sched.ev[a]->idx = a;
	ctx->sched.ev[b]->idx = b;
}

__attribute__((nonnull)) static void sift_up(struct p_ctx *const ctx,
					     size_t node)
{
	while (node) {
		const size_t parent = node_parent(node);

		if (ctx->sched.ev[parent]->ts <= ctx->sched.ev[node]->ts)
			break;

		heap_swap(ctx, parent, node);
		node = parent;
	}
}

__attribute__((nonnull)) static void sift_down(struct p_ctx *const ctx,
					       size_t node)
{
	for (;;) {
		size_t smallest = node;
		const size_t l = node_left(node);
		const size_t r = node_right(node);

		if ((l < ctx->sched.num_ev) &&
		    (ctx->sched.ev[l]->ts < ctx->sched.ev[smallest]->ts))
			smallest = l;

		if ((r < ctx->sched.num_ev) &&
		    ctx->sched.ev[r]->ts < ctx->sched.ev[smallest]->ts)
			smallest = r;

		if (smallest == node)
			break;

		heap_swap(ctx, node, smallest);
		node = smallest;
	}
}

void p_sched_rst(struct p_ctx *const ctx)
{
	memset(&ctx->sched, 0, sizeof(ctx->sched));
	LOG_INFO(ctx, "reset");
}

bool p_sched_run(struct p_ctx *const ctx)
{
	bool ev_ran = false;

	while ((ctx->sched.num_ev) &&
	       ctx->sched.ev[0]->ts <= ctx->sched.ts_now) {
		ev_ran = true;

		struct p_sched_ev *const ev = ctx->sched.ev[0];
		ev->cb(ctx);

		p_sched_del(ctx, ev);
	}
	return ev_ran;
}

void p_sched_add(struct p_ctx *const ctx, struct p_sched_ev *const ev)
{
	assert(ctx->sched.num_ev < ARRAY_SIZE(ctx->sched.ev));

	const size_t node = ctx->sched.num_ev;

	ev->idx = node;
	ev->valid = true;

	ctx->sched.ev[ctx->sched.num_ev++] = ev;

	sift_up(ctx, node);
}

void p_sched_del(struct p_ctx *const ctx, struct p_sched_ev *const ev)
{
	if (!ev->valid)
		return;

	const size_t idx = ev->idx;
	const size_t last = --ctx->sched.num_ev;
	ev->valid = false;

	if (idx != last) {
		ctx->sched.ev[idx] = ctx->sched.ev[last];
		ctx->sched.ev[idx]->idx = idx;

		sift_down(ctx, idx);
		sift_up(ctx, idx);
	}
}
