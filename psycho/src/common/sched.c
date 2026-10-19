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

#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "sched.h"
#include "debug/log.h"
#include "util.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_SCHED);

static const char *event_names[PSYCHO_SCHED_EVENT_TYPE_COUNT] = {
	[PSYCHO_SCHED_EVENT_TYPE_VBLANK]			 = "vblank",
	[PSYCHO_SCHED_EVENT_TYPE_SIO0_TX]			 = "sio0 tx",
	[PSYCHO_SCHED_EVENT_TYPE_SIO0_DEVICE_ACK_PULSE_ASSERT]	 = "sio0 device ack assert",
	[PSYCHO_SCHED_EVENT_TYPE_SIO0_DEVICE_ACK_PULSE_DEASSERT] = "sio0 device ack deassert"
};

PSYCHO_NODISCARD static size_t node_parent(size_t node)
{
	return (node - 1) / 2;
}

PSYCHO_NODISCARD static size_t node_left(size_t node)
{
	return (node * 2) + 1;
}

PSYCHO_NODISCARD static size_t node_right(size_t node)
{
	return node_left(node) + 1;
}

PSYCHO_NONNULL static void heap_swap(struct psycho_ctx *ctx, size_t a, size_t b)
{
	swap(&ctx->sched.events[a], &ctx->sched.events[b]);

	ctx->sched.events[a]->idx = a;
	ctx->sched.events[b]->idx = b;
}

PSYCHO_NONNULL static void sift_up(struct psycho_ctx *ctx, size_t node)
{
	while (node) {
		size_t parent = node_parent(node);

		if (ctx->sched.events[parent]->ts <= ctx->sched.events[node]->ts)
			break;

		heap_swap(ctx, parent, node);
		node = parent;
	}
}

PSYCHO_NONNULL static void sift_down(struct psycho_ctx *ctx, size_t node)
{
	for (;;) {
		size_t smallest = node;
		const size_t l	= node_left(node);
		const size_t r	= node_right(node);

		if ((l < ctx->sched.num_events) && (ctx->sched.events[l]->ts < ctx->sched.events[smallest]->ts))
			smallest = l;

		if ((r < ctx->sched.num_events) && ctx->sched.events[r]->ts < ctx->sched.events[smallest]->ts)
			smallest = r;

		if (smallest == node)
			break;

		heap_swap(ctx, node, smallest);
		node = smallest;
	}
}

void p_sched_rst(struct psycho_ctx *ctx)
{
	memset(&ctx->sched, 0, sizeof(ctx->sched));
	LOG_INFO(ctx, "reset");
}

void p_sched_run(struct psycho_ctx *ctx)
{
	struct psycho_sched_event *event = ctx->sched.events[0];

	LOG_TRACE(ctx, "servicing event \"%s\" (ts_now=%" PRIu64 ")", event_names[event->type], ctx->sched.ts_now);

	if (unlikely(event->permanent)) {
		event->ts += event->period;
		sift_down(ctx, event->idx);
	} else
		p_sched_del(ctx, event);

	event->cb(ctx, event->userdata);
}

void p_sched_add(struct psycho_ctx *ctx, struct psycho_sched_event *event)
{
	assert(ctx->sched.num_events < ARRAY_SIZE(ctx->sched.events));

	if (unlikely(ctx->sched.num_events >= ARRAY_SIZE(ctx->sched.events))) {
		LOG_ERR(ctx, "event heap full; dropping event \"%s\"", event_names[event->type]);
		return;
	}

	size_t node = ctx->sched.num_events;

	event->idx   = node;
	event->valid = true;

	if (unlikely(event->permanent)) {
		assert(event->ts > 0);
		event->period = event->ts;
	}

	event->ts += ctx->sched.ts_now;

	ctx->sched.events[ctx->sched.num_events++] = event;

	uint64_t expiry	   = event->ts - ctx->sched.ts_now;
	const char *plural = likely(expiry != 1) ? "s" : "";

	LOG_TRACE(ctx, "adding event \"%s\"; will be serviced within %" PRIu64 " cycle%s; ts_now=%" PRIu64,
		  event_names[event->type], expiry, plural, ctx->sched.ts_now);

	sift_up(ctx, node);
}

void p_sched_del(struct psycho_ctx *ctx, struct psycho_sched_event *event)
{
	if (unlikely(!event->valid))
		return;

	assert(!event->permanent);

	event->valid = false;

	size_t idx  = event->idx;
	size_t last = --ctx->sched.num_events;

	if (idx != last) {
		ctx->sched.events[idx]	    = ctx->sched.events[last];
		ctx->sched.events[idx]->idx = idx;

		sift_down(ctx, idx);
		sift_up(ctx, idx);
	}

	LOG_TRACE(ctx, "event \"%s\" deleted", event_names[event->type]);
}
