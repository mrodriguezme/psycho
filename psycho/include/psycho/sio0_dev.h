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
#include "sched.h"

struct p_ctx;

enum p_sio0_dev_type {
	P_SIO0_DEV_TYPE_CTRL,
	P_SIO0_DEV_TYPE_MEMCARD,
};

struct p_sio0_dev {
	struct p_ctx *ctx;
	void *handle;

	u8 (*transceive)(void *dev, u8 mosi);
	void (*reset)(void *dev);

	const char *name;
	enum p_sio0_dev_type type;

	struct p_sched_ev ack_pulse_begin_ev;
	struct p_sched_ev ack_pulse_end_ev;
};

void p_sio0_dev_ack(struct p_sio0_dev *dev, uint delay_us, uint pulse_us);
