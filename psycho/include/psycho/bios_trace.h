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

#include <stdbool.h>
#include <stddef.h>

#include "str.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct p_ctx;

enum {
	// clang-format off

	P_BIOS_TRACE_STACK_MAX		= 10,
	P_BIOS_TRACE_TTY_STR_LEN_MAX	= 512

	// clang-format on
};

enum p_bios_fn_ret {
	P_BIOS_FN_RET_INT,
	P_BIOS_FN_RET_CHAR,
	P_BIOS_FN_RET_VOID,
	P_BIOS_FN_RET_VOID_PTR
};

struct p_bios_frame {
	const struct p_bios_fn *fn;
	struct p_str str;
	char str_buf[512];

	u32 a0;
	u32 a1;
	u32 a2;
	u32 a3;
	u32 sp;
	u32 ra;
	u32 arg_pos;
};

struct p_bios_fn {
	const char *const prototype;
	const enum p_bios_fn_ret ret;
	void (*hook_cb)(struct p_ctx *ctx, const struct p_bios_frame *frame);
};

struct p_bios_trace_cfg {
	void (*stdout_line)(struct p_ctx *ctx, struct p_str *str);
	bool deref_ptrs;
};

struct p_tty_str {
	char buf[P_BIOS_TRACE_TTY_STR_LEN_MAX];
	struct p_str str;
};

struct p_bios_trace {
	struct {
		struct p_bios_frame frames[P_BIOS_TRACE_STACK_MAX];
		size_t top;
	} stack;

	struct p_tty_str tty_orig;
	struct p_tty_str tty_log;
};

#ifdef __cplusplus
}
#endif // __cplusplus
