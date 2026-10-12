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

#include <stdio.h>

#include "psycho/devices/digital_pad.h"
#include "debug/log.h"

LOG_MOD(P_LOG_DIGITAL_CTRL);

#define HI_Z	     (UINT8_MAX)

// Delay in microseconds between receiving a byte and asserting ACK.
#define ACK_DELAY_US (15)

// Duration, in microseconds, that ACK is asserted for.
#define ACK_PULSE_US (10)

#define CTRL_ID	     (0x5A41)

P_CONST static const char *button_name(enum psycho_digital_pad_button button)
{
	switch (button) {
	case PSYCHO_DIGITAL_PAD_BUTTON_SELECT:
		return "Select";

	case PSYCHO_DIGITAL_PAD_BUTTON_START:
		return "Start";

	case PSYCHO_DIGITAL_PAD_BUTTON_UP:
		return "Up";

	case PSYCHO_DIGITAL_PAD_BUTTON_RIGHT:
		return "Right";

	case PSYCHO_DIGITAL_PAD_BUTTON_DOWN:
		return "Down";

	case PSYCHO_DIGITAL_PAD_BUTTON_LEFT:
		return "Left";

	case PSYCHO_DIGITAL_PAD_BUTTON_L2:
		return "L2";

	case PSYCHO_DIGITAL_PAD_BUTTON_R2:
		return "R2";

	case PSYCHO_DIGITAL_PAD_BUTTON_L1:
		return "L1";

	case PSYCHO_DIGITAL_PAD_BUTTON_R1:
		return "R1";

	case PSYCHO_DIGITAL_PAD_BUTTON_TRIANGLE:
		return "Triangle";

	case PSYCHO_DIGITAL_PAD_BUTTON_CIRCLE:
		return "Circle";

	case PSYCHO_DIGITAL_PAD_BUTTON_CROSS:
		return "Cross";

	case PSYCHO_DIGITAL_PAD_BUTTON_SQUARE:
		return "Square";

	default:
		P_UNREACHABLE;
	}
}

static int button_count(enum psycho_digital_pad_button buttons)
{
	return __builtin_popcount((unsigned int)buttons);
}

static void button_list(enum psycho_digital_pad_button btns, char *buf, size_t buf_sz)
{
	size_t off = 0;
	bool first = true;

	buf[0] = '\0';

	for (unsigned int bit = 0; bit < 16; ++bit) {
		enum psycho_digital_pad_button cur = btns & (1 << bit);

		if (!cur)
			continue;

		off += snprintf(&buf[off], buf_sz - off, "%s%s", first ? "" : "/", button_name(cur));
		first = false;
	}

	if (first)
		snprintf(buf, buf_sz, "none");
}

static uint8_t transceive(void *dev, uint8_t mosi)
{
	struct psycho_digital_pad *pad = dev;

	uint8_t miso = HI_Z;

	bool ack = false;

	switch (pad->state) {
	case PSYCHO_DIGITAL_PAD_STATE_HI_Z:
		if (mosi != 0x01) {
			LOG_DBG(pad->dev.ctx, "not addressed");
			break;
		}

		pad->state = PSYCHO_DIGITAL_PAD_STATE_ID_LO;

		ack = true;
		break;

	case PSYCHO_DIGITAL_PAD_STATE_ID_LO:
		if (mosi != 0x42) {
			LOG_WARN(pad->dev.ctx, "expected 0x42, got 0x%02X", mosi);
			break;
		}

		pad->state = PSYCHO_DIGITAL_PAD_STATE_ID_HI;
		miso	   = CTRL_ID & UINT8_MAX;

		ack = true;
		break;

	case PSYCHO_DIGITAL_PAD_STATE_ID_HI:
		pad->state = PSYCHO_DIGITAL_PAD_STATE_SW_LO;
		miso	   = CTRL_ID >> 8;

		ack = true;
		break;

	case PSYCHO_DIGITAL_PAD_STATE_SW_LO:
		pad->state = PSYCHO_DIGITAL_PAD_STATE_SW_HI;
		miso	   = pad->buttons & UINT8_MAX;

		ack = true;
		break;

	case PSYCHO_DIGITAL_PAD_STATE_SW_HI:
		pad->state = PSYCHO_DIGITAL_PAD_STATE_HI_Z;
		miso	   = pad->buttons >> 8;

		// The last byte does not toggle ACK, confirmed by scope traces.
		ack = false;
		break;

	default:
		P_UNREACHABLE;
	}

	if (ack)
		p_sio0_dev_ack(&pad->dev, ACK_DELAY_US, ACK_PULSE_US);

	return miso;
}

static void reset(void *dev)
{
	struct psycho_digital_pad *pad = dev;

	pad->state = PSYCHO_DIGITAL_PAD_STATE_HI_Z;
}

void psycho_digital_pad_init(struct p_ctx *ctx, struct psycho_digital_pad *pad)
{
	pad->dev.ctx	= ctx;
	pad->dev.handle = pad;

	pad->dev.transceive = transceive;
	pad->dev.reset	    = reset;

	pad->dev.name = "Digital Pad (SCPH-1010)";
	pad->dev.type = P_SIO0_DEV_TYPE_CTRL;

	pad->buttons = UINT16_MAX;

	LOG_INFO(ctx, "spawned peripheral \"%s\"", pad->dev.name);
}

void psycho_digital_pad_buttons_press(struct psycho_digital_pad *pad, enum psycho_digital_pad_button buttons)
{
	pad->buttons &= ~buttons;

	char names[64];
	button_list(buttons, names, sizeof(names));

	LOG_INFO(pad->dev.ctx, "button%s \"%s\" pressed", button_count(buttons) == 1 ? "" : "s", names);
}

void psycho_digital_pad_buttons_release(struct psycho_digital_pad *pad, enum psycho_digital_pad_button buttons)
{
	char names[64];

	button_list(buttons, names, sizeof(names));
	pad->buttons |= buttons;

	LOG_INFO(pad->dev.ctx, "button%s \"%s\" released", button_count(buttons) == 1 ? "" : "s", names);
}
