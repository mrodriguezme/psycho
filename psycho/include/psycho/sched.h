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
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct p_ctx;

#define P_SCHED_NUM_EVENTS (25)

enum p_sched_ev_type {
	P_SCHED_EV_VBLANK,
	P_SCHED_EV_SIO0_TX,
	P_SCHED_EV_SIO0_DEV_ACK,
	P_SCHED_EV_COUNT,
};

struct p_sched_ev {
	u64 ts;
	u64 period;

	void (*cb)(struct p_ctx *ctx);
	enum p_sched_ev_type type;
	bool permanent;

	size_t idx;
	bool valid;
};

struct p_sched {
	struct p_sched_ev *ev[P_SCHED_NUM_EVENTS];
	size_t num_ev;
	u64 ts_now;
};

#ifdef __cplusplus
}
#endif // __cplusplus
