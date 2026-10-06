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

#include "psycho/compiler_support.h"
#include "sio0_device.h"

enum psycho_sio0_slot {
	PSYCHO_SIO0_SLOT_1,
	PSYCHO_SIO0_SLOT_2,
	PSYCHO_SIO0_SLOT_COUNT,
};

struct psycho_sio0 {
	struct psycho_sio0_device *devices[PSYCHO_SIO0_SLOT_COUNT][PSYCHO_SIO0_DEVICE_TYPE_COUNT];
	struct psycho_sio0_device *curr_device;

	struct {
		uint32_t entry;
		uint32_t latched;
	} txfifo;

	struct {
		size_t num_entries;

		union {
			uint8_t entries[4];
			uint32_t raw;
		};
	} rxfifo;

	uint32_t stat;
	uint16_t mode;
	uint16_t ctrl;
	uint16_t baud;

	uint8_t last_rx;

	struct psycho_scheduler_event tx_event;
};

void psycho_attach_device_to_sio0(struct psycho_ctx *ctx, struct psycho_sio0_device *dev,
				  enum psycho_sio0_slot slot) PSYCHO_NONNULL;
