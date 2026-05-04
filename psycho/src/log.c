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

void psycho_log_init(struct psycho_ctx *const ctx,
		     const struct psycho_log_cfg *const cfg)
{
	assert(ctx != NULL);
	assert(cfg != NULL);

	ctx->log.cfg = *cfg;
}

__attribute__((format(printf, 4, 5))) void
psycho_log_msg(struct psycho_ctx *const ctx,
	       const enum psycho_log_module module,
	       const enum psycho_log_level level, const char *const fmt, ...)
{
	assert(ctx != NULL);
	assert(ctx->log.cfg.log_cb != NULL);

	assert(module < PSYCHO_LOG_MODULE_COUNT);
	assert((level > PSYCHO_LOG_LEVEL_OFF) &&
	       (level < PSYCHO_LOG_LEVEL_COUNT));

	static const char *const level_str[PSYCHO_LOG_LEVEL_COUNT] = {
		// clang-format off

		[PSYCHO_LOG_LEVEL_INFO]		= "info",
		[PSYCHO_LOG_LEVEL_WARN]		= "warn",
		[PSYCHO_LOG_LEVEL_ERR]		= "error",
		[PSYCHO_LOG_LEVEL_DBG]		= "debug",
		[PSYCHO_LOG_LEVEL_TRACE]	= "trace"

		// clang-format on
	};

	static const char *const module_str[PSYCHO_LOG_MODULE_COUNT] = {
		// clang-format off

		[PSYCHO_LOG_MODULE_CTX]		= "ctx",
		[PSYCHO_LOG_MODULE_CPU]		= "cpu",
		[PSYCHO_LOG_MODULE_BUS]		= "bus",
		[PSYCHO_LOG_MODULE_BIOS]	= "bios"

		// clang-format on
	};

	char msg[PSYCHO_LOG_MSG_LEN_MAX];

	int len = snprintf(msg, sizeof(msg), "[%s/%s] ", level_str[level],
			   module_str[module]);

	va_list args;
	va_start(args, fmt);
	len += vsnprintf(&msg[len], sizeof(msg) - len, fmt, args);
	va_end(args);

	const struct psycho_log_msg_data msg_data = {
		// clang-format off

		.msg	= msg,
		.len	= len,
		.module	= module,
		.level	= level

		// clang-format on
	};
	ctx->log.cfg.log_cb(ctx, &msg_data);
}
