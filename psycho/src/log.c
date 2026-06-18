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

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "log.h"
#include "str.h"

void p_log_msg(struct p_ctx *const ctx, const enum p_log_mod mod,
	       const enum p_log_lvl lvl, const char *const fmt, ...)
{
	assert(ctx->cfg.log.log_cb != NULL);

	assert(mod < P_LOG_MOD_COUNT);
	assert((lvl > P_LOG_OFF) && (lvl < P_LOG_COUNT));

	static const char *const lvl_str[P_LOG_COUNT] = {
		// clang-format off

		[P_LOG_INFO]	= "info",
		[P_LOG_WARN]	= "warn",
		[P_LOG_ERR]	= "err",
		[P_LOG_DBG]	= "dbg",
		[P_LOG_TRACE]	= "trace"

		// clang-format on
	};

	static const char *const mod_str[P_LOG_MOD_COUNT] = {
		// clang-format off

		[P_LOG_CTX]	= "ctx",
		[P_LOG_CPU]	= "cpu",
		[P_LOG_BUS]	= "bus",
		[P_LOG_BIOS]	= "bios"

		// clang-format on
	};

	char buf[512];
	struct p_str str;
	p_str_init_fixed(&str, buf, sizeof(buf));

	p_str_append(&str, NULL, "[%s/%s] ", lvl_str[lvl], mod_str[mod]);

	va_list args;
	va_start(args, fmt);
	p_str_vappend(&str, NULL, fmt, args);
	va_end(args);

	const struct p_log_msg msg = {
		// clang-format off

		.str	= str,
		.mod	= mod,
		.lvl	= lvl

		// clang-format on
	};
	ctx->cfg.log.log_cb(ctx, &msg);
}
