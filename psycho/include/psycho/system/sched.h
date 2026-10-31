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

#include <stdint.h>
#include <stddef.h>
#include "psycho/lib/binary_heap.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct psycho_ctx;

constexpr size_t PSYCHO_SCHED_EVENTS_NUM_MAX = 25;

PSYCHO_BINARY_HEAP_DEFINE(psycho_sched_event_heap, struct psycho_sched_event *, PSYCHO_SCHED_EVENTS_NUM_MAX);

#define PSYCHO_SCHED_EVENT_LIST(X)                                      \
	X(VBLANK, "vblank")                                             \
	X(SIO0_TX, "sio0 tx")                                           \
	X(SIO0_DEVICE_ACK_PULSE_ASSERT, "sio0 device ack pulse assert") \
	X(SIO0_DEVICE_ACK_PULSE_DEASSERT, "sio0 device ack pulse deassert")

enum psycho_sched_event_type {
#define X(id, name) PSYCHO_SCHED_EVENT_TYPE_##id,
	PSYCHO_SCHED_EVENT_LIST(X)
#undef X
	// clang-format off

	PSYCHO_SCHED_EVENT_COUNT

	// clang-format on
};

struct psycho_sched_event {
	uint64_t ts;
	uint64_t period;

	void (*cb)(struct psycho_ctx *ctx, void *userdata);
	void *userdata;

	enum psycho_sched_event_type type;

	bool permanent;
	bool valid;
};

struct psycho_sched {
	struct psycho_sched_event_heap heap;
	uint64_t ts_now;
};

#ifdef __cplusplus
}
#endif // __cplusplus
