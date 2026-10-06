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
#include <stddef.h>
#include <stdint.h>

#include "fixed_string.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct psycho_ctx;

enum {
	PSYCHO_BIOS_FUNC_TRACER_MAX_CALL_STACK_DEPTH = 10,
	PSYCHO_BIOS_FUNC_TRACER_MAX_TTY_STDOUT_LEN   = 512,
};

enum psycho_bios_func_tracer_func_ret_type {
	PSYCHO_BIOS_FUNC_TRACER_FUNC_RET_TYPE_INT,
	PSYCHO_BIOS_FUNC_TRACER_FUNC_RET_TYPE_CHAR,
	PSYCHO_BIOS_FUNC_TRACER_FUNC_RET_TYPE_VOID,
	PSYCHO_BIOS_FUNC_TRACER_FUNC_RET_TYPE_VOID_PTR
};

struct psycho_bios_func_tracer_stack_frame {
	const struct psycho_bios_func_tracer_func_data *func;
	struct psycho_fixed_string str;
	char str_buf[512];

	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	uint32_t a3;
	uint32_t sp;
	uint32_t ra;
};

struct psycho_bios_func_tracer_func_data {
	const char *prototype;
	const enum psycho_bios_func_tracer_func_ret_type ret;
	void (*hook_cb)(struct psycho_ctx *ctx, struct psycho_bios_func_tracer_stack_frame *frame);
};

struct psycho_bios_func_tracer_cfg {
	void (*stdout_line)(struct psycho_ctx *ctx, struct psycho_fixed_string *str);
	bool deref_ptrs;
};

struct psycho_tty_str {
	char data[PSYCHO_BIOS_FUNC_TRACER_MAX_TTY_STDOUT_LEN];
	struct psycho_fixed_string str;
};

struct psycho_bios_func_tracer {
	struct {
		struct psycho_bios_func_tracer_stack_frame frames[PSYCHO_BIOS_FUNC_TRACER_MAX_CALL_STACK_DEPTH];
		size_t top;
	} stack;

	struct psycho_tty_str tty_orig;
	struct psycho_tty_str tty_log;
};

#ifdef __cplusplus
}
#endif // __cplusplus
