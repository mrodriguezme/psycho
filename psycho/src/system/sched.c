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

#include "sched.h"
#include "lib/binary_heap.h"
#include "debug/log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_SCHED);

static const char *const event_names[PSYCHO_SCHED_EVENT_COUNT] = {
#define X(id, desc) [PSYCHO_SCHED_EVENT_TYPE_##id] = (desc),
	PSYCHO_SCHED_EVENT_LIST(X)
#undef X
};

static bool cmp_func(struct psycho_sched_event *a, struct psycho_sched_event *b)
{
	return a->ts < b->ts;
}

BINARY_HEAP_IMPL(psycho_sched_event_heap, struct psycho_sched_event *, cmp_func);

void psycho_sched_reset(struct psycho_ctx *ctx)
{
	psycho_sched_event_heap_reset(&ctx->sched.heap);
	ctx->sched.ts_now = 0;

	LOG_INFO(ctx, "reset");
}

void psycho_sched_run(struct psycho_ctx *ctx)
{
	struct psycho_sched_event *event = psycho_sched_event_heap_root(&ctx->sched.heap);

	LOG_TRACE(ctx, "servicing event \"%s\" (ts_now=%" PRIu64 ")", event_names[event->type], ctx->sched.ts_now);

	if (likely(!event->permanent)) {
		psycho_sched_event_heap_delete_at(&ctx->sched.heap, 0);
		event->valid = false;
	} else {
		event->ts += event->period;
		psycho_sched_event_heap_sift_down(&ctx->sched.heap, 0);
	}

	event->cb(ctx, event->userdata);
}

void psycho_sched_event_add(struct psycho_ctx *ctx, struct psycho_sched_event *event)
{
	if (unlikely(psycho_sched_event_heap_full(&ctx->sched.heap))) {
		LOG_ERR(ctx, "event heap full; dropping event \"%s\"", event_names[event->type]);
		return;
	}

	event->valid = true;

	if (unlikely(event->permanent)) {
		assert(event->ts > 0);
		event->period = event->ts;
	}

	event->ts += ctx->sched.ts_now;

	uint64_t expiry	   = event->ts - ctx->sched.ts_now;
	const char *plural = likely(expiry != 1) ? "s" : "";

	LOG_TRACE(ctx, "adding event \"%s\"; will be serviced within %" PRIu64 " cycle%s; ts_now=%" PRIu64,
		  event_names[event->type], expiry, plural, ctx->sched.ts_now);

	psycho_sched_event_heap_insert(&ctx->sched.heap, event);
}

void psycho_sched_event_delete(struct psycho_ctx *ctx, struct psycho_sched_event *event)
{
	if (unlikely(!event->valid))
		return;

	psycho_sched_event_heap_delete(&ctx->sched.heap, event);
	event->valid = false;

	LOG_TRACE(ctx, "event \"%s\" deleted", event_names[event->type]);
}
