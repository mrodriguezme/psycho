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

#include "sched.h"
#include "util.h"
#include "devices/internal/sio0.h"
#include "log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_SIO0);

void psycho_sio0_device_ack(struct psycho_sio0_device *dev, unsigned int delay_us, unsigned int pulse_us)
{
	dev->ack_pulse_begin_event.cb	    = psycho_sio0_dsr_assert;
	dev->ack_pulse_begin_event.ts	    = us_to_cycles(delay_us);
	dev->ack_pulse_begin_event.type	    = PSYCHO_SCHEDULER_EVENT_TYPE_SIO0_DEVICE_ACK_PULSE_BEGIN;
	dev->ack_pulse_begin_event.userdata = dev;

	dev->ack_pulse_end_event.cb   = psycho_sio0_dsr_deassert;
	dev->ack_pulse_end_event.ts   = us_to_cycles(delay_us + pulse_us);
	dev->ack_pulse_end_event.type = PSYCHO_SCHEDULER_EVENT_TYPE_SIO0_DEVICE_ACK_PULSE_END;

	psycho_scheduler_add_event(dev->ctx, &dev->ack_pulse_begin_event);
	psycho_scheduler_add_event(dev->ctx, &dev->ack_pulse_end_event);
}
