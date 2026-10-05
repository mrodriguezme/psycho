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

#include "psycho/compiler.h"
#include "psycho/sio0_device.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum psycho_digital_controller_state {
	PSYCHO_DIGITAL_CONTROLLER_STATE_HI_Z,
	PSYCHO_DIGITAL_CONTROLLER_STATE_ID_LO,
	PSYCHO_DIGITAL_CONTROLLER_STATE_ID_HI,
	PSYCHO_DIGITAL_CONTROLLER_STATE_SW_LO,
	PSYCHO_DIGITAL_CONTROLLER_STATE_SW_HI
};

enum psycho_digital_controller_button {
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_SELECT	    = 1 << 0,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_START	    = 1 << 3,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_DPAD_UP    = 1 << 4,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_DPAD_RIGHT = 1 << 5,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_DPAD_DOWN  = 1 << 6,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_DPAD_LEFT  = 1 << 7,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_L2	    = 1 << 8,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_R2	    = 1 << 9,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_L1	    = 1 << 10,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_R1	    = 1 << 11,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_TRIANGLE   = 1 << 12,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_CIRCLE	    = 1 << 13,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_CROSS	    = 1 << 14,
	PSYCHO_DIGITAL_CONTROLLER_BUTTON_SQUARE	    = 1 << 15
};

struct psycho_digital_controller {
	struct psycho_sio0_device device;
	enum psycho_digital_controller_state state;
	uint16_t buttons;
};

void psycho_digital_controller_init(struct psycho_ctx *ctx, struct psycho_digital_controller *ctrl) PSYCHO_NONNULL;

void psycho_digital_controller_press_buttons(struct psycho_digital_controller *ctrl,
					     enum psycho_digital_controller_button buttons) PSYCHO_NONNULL;

void psycho_digital_controller_release_buttons(struct psycho_digital_controller *ctrl,
					       enum psycho_digital_controller_button buttons) PSYCHO_NONNULL;

#ifdef __cplusplus
}
#endif // __cplusplus
