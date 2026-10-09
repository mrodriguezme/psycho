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

#include <stddef.h>

#include "psycho/common/compiler_support.h"
#include "sio0_dev.h"
#include "sched.h"

enum sio0_slot {
	SLOT_1,
	SLOT_2,
	NUM_SLOTS,
};

enum sio0_dev_type {
	MEMCARD,
	CTRL,
	NUM_DEVS,
};

struct p_sio0 {
	struct p_sio0_dev *dev[NUM_SLOTS][NUM_DEVS];
	struct p_sio0_dev *curr_dev;

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

	struct p_sched_ev tx_ev;
};

void p_attach_dev_to_sio0(struct p_ctx *ctx, struct p_sio0_dev *dev, enum sio0_slot slot) P_NONNULL;
