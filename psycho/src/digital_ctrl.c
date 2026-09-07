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

#include <stdio.h>

#include "psycho/digital_ctrl.h"
#include "log.h"

LOG_MOD(P_LOG_DIGITAL_CTRL);

#define HI_Z	     (UINT8_MAX)

// Delay in microseconds between receiving a byte and asserting ACK.
#define ACK_DELAY_US (15)

// Duration, in microseconds, that ACK is asserted for.
#define ACK_PULSE_US (10)

#define CTRL_ID	     (0x5A41)

P_CONST static const char *btn_name(enum p_digital_ctrl_btns btn)
{
	switch (btn) {
	case P_DIGITAL_CTRL_SEL:
		return "Select";

	case P_DIGITAL_CTRL_START:
		return "Start";

	case P_DIGITAL_CTRL_UP:
		return "Up";

	case P_DIGITAL_CTRL_RT:
		return "Right";

	case P_DIGITAL_CTRL_DN:
		return "Down";

	case P_DIGITAL_CTRL_LT:
		return "Left";

	case P_DIGITAL_CTRL_L2:
		return "L2";

	case P_DIGITAL_CTRL_R2:
		return "R2";

	case P_DIGITAL_CTRL_L1:
		return "L1";

	case P_DIGITAL_CTRL_R1:
		return "R1";

	case P_DIGITAL_CTRL_TRI:
		return "Triangle";

	case P_DIGITAL_CTRL_CIR:
		return "Circle";

	case P_DIGITAL_CTRL_CROSS:
		return "Cross";

	case P_DIGITAL_CTRL_SQR:
		return "Square";

	default:
		P_UNREACHABLE;
	}
}

static int btn_count(enum p_digital_ctrl_btns btns)
{
	return __builtin_popcount((uint)btns);
}

static void btn_list(enum p_digital_ctrl_btns btns, char *buf, size_t buf_sz)
{
	size_t off = 0;
	bool first = true;

	buf[0] = '\0';

	for (u16 bit = 0; bit < 16; ++bit) {
		enum p_digital_ctrl_btns cur = btns & (1 << bit);

		if (!cur)
			continue;

		off += snprintf(&buf[off], buf_sz - off, "%s%s",
				first ? "" : "/", btn_name(cur));

		first = false;
	}

	if (first)
		snprintf(buf, buf_sz, "none");
}

static u8 transceive(void *dev, u8 mosi)
{
	struct p_digital_ctrl *ctrl = dev;

	u8 miso	 = HI_Z;
	bool ack = false;

	switch (ctrl->state) {
	case P_DIGITAL_CTRL_HI_Z:
		if (mosi != 0x01) {
			LOG_DBG(ctrl->dev.ctx, "not addressed");
			break;
		}

		ctrl->state = P_DIGITAL_CTRL_ID_LO;
		ack	    = true;

		break;

	case P_DIGITAL_CTRL_ID_LO:
		if (mosi != 0x42) {
			LOG_WARN(ctrl->dev.ctx, "expected 0x42, got 0x%02X",
				 mosi);
			break;
		}

		ctrl->state = P_DIGITAL_CTRL_ID_HI;
		miso	    = CTRL_ID & UINT8_MAX;

		ack = true;
		break;

	case P_DIGITAL_CTRL_ID_HI:
		ctrl->state = P_DIGITAL_CTRL_SW_LO;
		miso	    = CTRL_ID >> 8;

		ack = true;
		break;

	case P_DIGITAL_CTRL_SW_LO:
		ctrl->state = P_DIGITAL_CTRL_SW_HI;
		miso	    = ctrl->btns & UINT8_MAX;

		ack = true;
		break;

	case P_DIGITAL_CTRL_SW_HI:
		ctrl->state = P_DIGITAL_CTRL_HI_Z;
		miso	    = ctrl->btns >> 8;

		// The last byte does not toggle ACK, confirmed by scope traces.
		ack = false;
		break;

	default:
		P_UNREACHABLE;
	}

	if (ack)
		p_sio0_dev_ack(&ctrl->dev, ACK_DELAY_US, ACK_PULSE_US);

	return miso;
}

static void reset(void *dev)
{
	struct p_digital_ctrl *ctrl = dev;

	ctrl->state = P_DIGITAL_CTRL_HI_Z;
}

void p_digital_ctrl_init(struct p_ctx *ctx, struct p_digital_ctrl *ctrl)
{
	ctrl->dev.ctx	 = ctx;
	ctrl->dev.handle = ctrl;

	ctrl->dev.transceive = transceive;
	ctrl->dev.reset	     = reset;

	ctrl->dev.name = "Digital Controller (SCPH-1010)";
	ctrl->dev.type = P_SIO0_DEV_TYPE_CTRL;

	ctrl->btns = UINT16_MAX;

	LOG_INFO(ctx, "spawned peripheral \"%s\"", ctrl->dev.name);
}

void p_digital_ctrl_btn_press(struct p_digital_ctrl *ctrl,
			      enum p_digital_ctrl_btns btns)
{
	char names[64];
	btn_list(btns, names, sizeof(names));

	ctrl->btns &= ~btns;

	LOG_INFO(ctrl->dev.ctx, "button%s \"%s\" pressed",
		 btn_count(btns) == 1 ? "" : "s", names);
}

void p_digital_ctrl_btn_rel(struct p_digital_ctrl *ctrl,
			    enum p_digital_ctrl_btns btns)
{
	char names[64];

	btn_list(btns, names, sizeof(names));
	ctrl->btns |= btns;

	LOG_INFO(ctrl->dev.ctx, "button%s \"%s\" released",
		 btn_count(btns) == 1 ? "" : "s", names);
}
