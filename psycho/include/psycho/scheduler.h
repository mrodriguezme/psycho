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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct psycho_ctx;

enum {
	PSYCHO_SCHEDULER_MAX_NUM_EVENTS = 25
};

enum psycho_scheduler_event_type {
	PSYCHO_SCHEDULER_EVENT_TYPE_VBLANK,
	PSYCHO_SCHEDULER_EVENT_TYPE_SIO0_TX,
	PSYCHO_SCHEDULER_EVENT_TYPE_SIO0_DEVICE_ACK_PULSE_BEGIN,
	PSYCHO_SCHEDULER_EVENT_TYPE_SIO0_DEVICE_ACK_PULSE_END,
	PSYCHO_SCHEDULER_EVENT_TYPE_COUNT
};

struct psycho_scheduler_event {
	uint64_t ts;
	uint64_t period;

	void (*cb)(struct psycho_ctx *ctx, void *userdata);
	enum psycho_scheduler_event_type type;
	bool permanent;

	size_t idx;
	bool valid;

	void *userdata;
};

struct psycho_scheduler {
	struct psycho_scheduler_event *events[PSYCHO_SCHEDULER_MAX_NUM_EVENTS];
	size_t num_ev;
	uint64_t ts_now;
};

#ifdef __cplusplus
}
#endif // __cplusplus
