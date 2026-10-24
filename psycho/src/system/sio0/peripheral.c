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

#include "system/sched.h"
#include "common/util.h"

#include "debug/log.h"

#include "sio0.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_SIO0);

void psycho_sio0_device_init(struct psycho_sio0_device *dev)
{
	LOG_INFO(dev->ctx, "device \"%s\" initialized", dev->name);
}

void psycho_sio0_device_ack(struct psycho_sio0_device *dev, unsigned int delay_us, unsigned int pulse_us)
{
	dev->ack_pulse_assert_event.cb	     = psycho_sio0_dsr_assert;
	dev->ack_pulse_assert_event.ts	     = us_to_cycles(delay_us);
	dev->ack_pulse_assert_event.type     = PSYCHO_SCHED_EVENT_TYPE_SIO0_DEVICE_ACK_PULSE_ASSERT;
	dev->ack_pulse_assert_event.userdata = dev;

	dev->ack_pulse_deassert_event.cb   = psycho_sio0_dsr_deassert;
	dev->ack_pulse_deassert_event.ts   = us_to_cycles(delay_us + pulse_us);
	dev->ack_pulse_deassert_event.type = PSYCHO_SCHED_EVENT_TYPE_SIO0_DEVICE_ACK_PULSE_DEASSERT;

	psycho_sched_event_add(dev->ctx, &dev->ack_pulse_assert_event);
	psycho_sched_event_add(dev->ctx, &dev->ack_pulse_deassert_event);
}

void psycho_sio0_device_attach(struct psycho_sio0_device *dev, enum psycho_sio0_slot slot)
{
	if (dev->ctx->sio0.devices[slot][dev->type]) {
		LOG_WARN(dev->ctx, "attempting to attach device of same type to the same port");
		return;
	}

	dev->ctx->sio0.devices[slot][dev->type] = dev;

	slot++;
	const char *type = (dev->type == PSYCHO_SIO0_DEVICE_TYPE_PAD) ? "pad" : "memcard";

	LOG_INFO(dev->ctx, "connected device \"%s\" to slot %u (type = %s)", dev->name, slot, type);
}
