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

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "log.h"
#include "str.h"

static const char *const level_names[PSYCHO_LOG_LEVEL_COUNT] = {
#define X(id, name) [PSYCHO_LOG_LEVEL_##id] = name,
	PSYCHO_LOG_LEVEL_LIST(X)
#undef X
};

static const char *const module_names[PSYCHO_LOG_MODULE_COUNT] = {
#define X(id, name) [PSYCHO_LOG_MODULE_##id] = name,
	PSYCHO_LOG_MODULE_LIST(X)
#undef X
};

void psycho_log_msg(struct p_ctx *ctx, enum psycho_log_module module, enum psycho_log_level level, const char *fmt, ...)
{
	assert(ctx->cfg.log.log_cb != NULL);
	assert(module < PSYCHO_LOG_MODULE_COUNT);
	assert((level > PSYCHO_LOG_LEVEL_OFF) && (level < PSYCHO_LOG_LEVEL_COUNT));

	char buf[512];
	struct p_str str;

	p_str_init_fixed(&str, buf, sizeof(buf));
	p_str_append(&str, NULL, "[%s/%s] ", level_names[level], module_names[module]);

	va_list args;
	va_start(args, fmt);
	p_str_vappend(&str, NULL, fmt, args);
	va_end(args);

	struct p_log_msg msg = {
		.str	= str,
		.module = module,
		.level	= level,
	};

	ctx->cfg.log.log_cb(ctx, &msg);
}
