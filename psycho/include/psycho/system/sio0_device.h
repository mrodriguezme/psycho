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

#include <stdbool.h>

#include "psycho/common/compiler_support.h"
#include "psycho/common/sched.h"

struct p_ctx;

enum psycho_sio0_slot {
	PSYCHO_SIO0_SLOT_1,
	PSYCHO_SIO0_SLOT_2,
	PSYCHO_SIO0_SLOT_COUNT
};

enum psycho_sio0_device_type {
	PSYCHO_SIO0_DEVICE_TYPE_PAD,
	PSYCHO_SIO0_DEVICE_TYPE_MEMCARD,
	PSYCHO_SIO0_DEVICE_TYPE_COUNT
};

struct psycho_sio0_device {
	struct p_ctx *ctx;
	void *handle;

	uint8_t (*transceive)(void *dev, uint8_t mosi);
	void (*reset)(void *dev);

	const char *name;
	enum psycho_sio0_device_type type;

	struct p_sched_ev ack_pulse_begin_ev;
	struct p_sched_ev ack_pulse_end_ev;
};

void psycho_sio0_device_init(struct psycho_sio0_device *dev);
void psycho_sio0_device_attach(struct psycho_sio0_device *dev, enum psycho_sio0_slot slot) P_NONNULL;
void psycho_sio0_device_ack(struct psycho_sio0_device *dev, unsigned int delay_us, unsigned int pulse_us);
