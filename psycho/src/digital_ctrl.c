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

#include "psycho/digital_ctrl.h"
#include "log.h"

LOG_MOD(P_LOG_DIGITAL_CTRL);

#define CTRL_ID (0x5A41)

static bool addressed(u8 addr)
{
	return addr == 0x01;
}

static u8 transceive(void *dev, u8 mosi)
{
	struct p_digital_ctrl *ctrl = dev;

	if (!ctrl->dev.active)
		return 0xFF;

	switch (ctrl->state) {
	case P_DIGITAL_CTRL_ID_LO:
		if (mosi == 0x42) {
			ctrl->latched_btns = ctrl->btns;

			ctrl->state = P_DIGITAL_CTRL_ID_HI;
			return CTRL_ID & UINT8_MAX;
		}
		return 0xFF;

	case P_DIGITAL_CTRL_ID_HI:
		ctrl->state = P_DIGITAL_CTRL_SW_LO;
		return CTRL_ID >> 8;

	case P_DIGITAL_CTRL_SW_LO:
		ctrl->state = P_DIGITAL_CTRL_SW_HI;
		return ctrl->latched_btns & UINT8_MAX;

	case P_DIGITAL_CTRL_SW_HI:
		ctrl->state = P_DIGITAL_CTRL_ID_LO;
		return ctrl->latched_btns >> 8;

	default:
		__builtin_unreachable();
	}
}

static void reset(void *dev)
{
	struct p_digital_ctrl *ctrl = dev;
	ctrl->state = P_DIGITAL_CTRL_ID_LO;
}

void p_digital_ctrl_init(struct p_ctx *ctx, struct p_digital_ctrl *ctrl)
{
	ctrl->ctx = ctx;

	ctrl->dev.transceive = transceive;
	ctrl->dev.name	     = "Digital Controller (SCPH-1010)";
	ctrl->dev.type	     = P_SIO0_DEV_TYPE_CTRL;
	ctrl->dev.addressed  = addressed;
	ctrl->dev.handle     = ctrl;
	ctrl->dev.reset	     = reset;

	ctrl->btns = UINT16_MAX;

	LOG_INFO(ctx, "spawned peripheral \"%s\"", ctrl->dev.name);
}

void p_digital_ctrl_btn_press(struct p_digital_ctrl *ctrl,
			      enum p_digital_ctrl_btns btns)
{
	ctrl->btns &= ~btns;
	LOG_INFO(ctrl->ctx, "button pressed");
}

void p_digital_ctrl_btn_rel(struct p_digital_ctrl *ctrl,
			    enum p_digital_ctrl_btns btns)
{
	ctrl->btns |= btns;
	LOG_INFO(ctrl->ctx, "button released");
}
