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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct psycho_ctx;

enum {
	PSYCHO_BIOS_TRACE_STACK_MAX = 10,
	PSYCHO_BIOS_TRACE_RESULT_LEN_MAX = 512,
};

enum psycho_bios_func_ret {
	PSYCHO_BIOS_FUNC_RET_INT,
	PSYCHO_BIOS_FUNC_RET_CHAR,
	PSYCHO_BIOS_FUNC_RET_VOID,
	PSYCHO_BIOS_FUNC_RET_VOID_PTR
};

struct psycho_bios_func {
	const char *const prototype;
	const enum psycho_bios_func_ret ret;
};

struct psycho_bios_frame {
	const struct psycho_bios_func *func;
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	uint32_t a3;
	uint32_t sp;
	uint32_t ra;

	struct {
		char result[PSYCHO_BIOS_TRACE_RESULT_LEN_MAX];
		size_t len;
	};
};

struct psycho_bios_trace_cfg {
	void (*tty_stdout_msg_cb)(struct psycho_ctx *, const char *msg);
	bool deref_ptrs;
};

struct psycho_bios_trace {
	struct {
		struct psycho_bios_frame frames[PSYCHO_BIOS_TRACE_STACK_MAX];
		size_t top;
	} stack;

	struct psycho_bios_trace_cfg cfg;
};

#ifdef __cplusplus
}
#endif // __cplusplus
