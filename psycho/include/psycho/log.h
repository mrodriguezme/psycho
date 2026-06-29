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

#include <stddef.h>
#include "str.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct p_ctx;

enum p_log_lvl {
	P_LOG_OFF,
	P_LOG_INFO,
	P_LOG_WARN,
	P_LOG_ERR,
	P_LOG_DBG,
	P_LOG_TRACE,
	P_LOG_COUNT
};

enum p_log_mod {
	P_LOG_CTX,
	P_LOG_CPU,
	P_LOG_BUS,
	P_LOG_BIOS,
	P_LOG_SCHED,
	P_LOG_GPU,
	P_LOG_INTCTRL,
	P_LOG_MOD_COUNT,
};

struct p_log_msg {
	const struct p_str str;
	const enum p_log_mod mod;
	const enum p_log_lvl lvl;
};

struct p_log_cfg {
	void (*log_cb)(struct p_ctx *ctx, const struct p_log_msg *msg);
	enum p_log_lvl mod[P_LOG_MOD_COUNT];
};

#ifdef __cplusplus
}
#endif // __cplusplus
