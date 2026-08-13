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
#include <inttypes.h>
#include <string.h>

#include "sched.h"
#include "log.h"
#include "util.h"

LOG_MOD(P_LOG_SCHED);

static const char *ev_names[P_SCHED_EV_COUNT] = {
	[P_SCHED_EV_VBLANK]  = "vblank",
	[P_SCHED_EV_SIO0_TX] = "sio0 tx"
};

P_NODISCARD static size_t node_parent(size_t node)
{
	return (node - 1) / 2;
}

P_NODISCARD static size_t node_left(size_t node)
{
	return (node * 2) + 1;
}

P_NODISCARD static size_t node_right(size_t node)
{
	return node_left(node) + 1;
}

P_NONNULL static void heap_swap(struct p_ctx *ctx, size_t a, size_t b)
{
	swap(&ctx->sched.ev[a], &ctx->sched.ev[b]);

	ctx->sched.ev[a]->idx = a;
	ctx->sched.ev[b]->idx = b;
}

P_NONNULL static void sift_up(struct p_ctx *ctx, size_t node)
{
	while (node) {
		size_t parent = node_parent(node);

		if (ctx->sched.ev[parent]->ts <= ctx->sched.ev[node]->ts)
			break;

		heap_swap(ctx, parent, node);
		node = parent;
	}
}

P_NONNULL static void sift_down(struct p_ctx *ctx, size_t node)
{
	for (;;) {
		size_t smallest = node;
		const size_t l	= node_left(node);
		const size_t r	= node_right(node);

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

void p_sched_rst(struct p_ctx *ctx)
{
	memset(&ctx->sched, 0, sizeof(ctx->sched));
	LOG_INFO(ctx, "reset");
}

bool p_sched_run(struct p_ctx *ctx)
{
	bool ev_ran = false;

	while (likely(ctx->sched.num_ev) &&
	       (ctx->sched.ev[0]->ts <= ctx->sched.ts_now)) {
		ev_ran = true;

		struct p_sched_ev *ev = ctx->sched.ev[0];
		u64 latency	      = ctx->sched.ts_now - ev->ts;

		LOG_TRACE(ctx,
			  "servicing event \"%s\" (ts_now=%" PRIu64 "), "
			  "latency=%" PRIu64,
			  ev_names[ev->type], ctx->sched.ts_now, latency);

		if (unlikely(ev->permanent)) {
			ev->ts += ev->period;
			sift_down(ctx, ev->idx);
		} else
			p_sched_del(ctx, ev);

		ev->cb(ctx);
	}

	return ev_ran;
}

void p_sched_add(struct p_ctx *ctx, struct p_sched_ev *ev)
{
	assert(ctx->sched.num_ev < ARRAY_SIZE(ctx->sched.ev));

	size_t node = ctx->sched.num_ev;

	ev->idx	  = node;
	ev->valid = true;

	if (unlikely(ev->permanent)) {
		assert(ev->ts > 0);
		ev->period = ev->ts;
	}

	ev->ts += ctx->sched.ts_now;

	ctx->sched.ev[ctx->sched.num_ev++] = ev;

	u64 expiry	   = ev->ts - ctx->sched.ts_now;
	const char *plural = likely(expiry != 1) ? "s" : "";

	LOG_TRACE(ctx,
		  "adding event \"%s\"; will be serviced within %lu cycle%s; "
		  "ts_now=%lu",
		  ev_names[ev->type], expiry, plural, ctx->sched.ts_now);

	sift_up(ctx, node);
}

void p_sched_del(struct p_ctx *ctx, struct p_sched_ev *ev)
{
	if (unlikely(!ev->valid))
		return;

	assert(ev->permanent != true);

	size_t idx  = ev->idx;
	size_t last = --ctx->sched.num_ev;

	ev->valid = false;

	if (idx != last) {
		ctx->sched.ev[idx]	= ctx->sched.ev[last];
		ctx->sched.ev[idx]->idx = idx;

		sift_down(ctx, idx);
		sift_up(ctx, idx);
	}

	LOG_TRACE(ctx, "event \"%s\" deleted", ev_names[ev->type]);
}
