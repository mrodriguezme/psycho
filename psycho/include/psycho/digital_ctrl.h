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

#include "compiler.h"
#include "sio0_dev.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum p_digital_ctrl_state {
	P_DIGITAL_CTRL_STATE_ID_LO,
	P_DIGITAL_CTRL_STATE_ID_HI,
	P_DIGITAL_CTRL_STATE_SW_LO,
	P_DIGITAL_CTRL_STATE_SW_HI
};

enum p_digital_ctrl_btns {
	// clang-format off

	P_DIGITAL_CTRL_SEL	= 1 << 0,
	P_DIGITAL_CTRL_START	= 1 << 3,
	P_DIGITAL_CTRL_UP	= 1 << 4,
	P_DIGITAL_CTRL_RT	= 1 << 5,
	P_DIGITAL_CTRL_DN	= 1 << 6,
	P_DIGITAL_CTRL_LT	= 1 << 7,
	P_DIGITAL_CTRL_L2	= 1 << 8,
	P_DIGITAL_CTRL_R2	= 1 << 9,
	P_DIGITAL_CTRL_L1	= 1 << 10,
	P_DIGITAL_CTRL_R1	= 1 << 11,
	P_DIGITAL_CTRL_TRI	= 1 << 12,
	P_DIGITAL_CTRL_CIR	= 1 << 13,
	P_DIGITAL_CTRL_CROSS	= 1 << 14,
	P_DIGITAL_CTRL_SQR	= 1 << 15

	// clang-format on
};

struct p_digital_ctrl {
	struct p_ctx *ctx;
	struct p_sio0_dev dev;
	enum p_digital_ctrl_state state;
	u16 buttons;
};

void p_digital_ctrl_init(struct p_ctx *ctx, struct p_digital_ctrl *ctrl);

void p_digital_ctrl_btn_press(struct p_digital_ctrl *ctrl,
			      const enum p_digital_ctrl_btns btns)
	__attribute__((nonnull));

void p_digital_ctrl_button_rel(struct p_digital_ctrl *ctrl,
			       const enum p_digital_ctrl_btns btns)
	__attribute__((nonnull));

#ifdef __cplusplus
}
#endif // __cplusplus
