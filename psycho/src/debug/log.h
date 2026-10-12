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

#pragma once

#include "psycho/ctx.h"

#define DECLARE_LOG_MODULE(mod) static const enum psycho_log_module m_log_module = (mod)

P_NODISCARD P_ALWAYS_INLINE P_NONNULL bool psycho_log_enabled(const struct p_ctx *ctx, enum psycho_log_module module,
							      enum psycho_log_level level)
{
	return ctx->cfg.log.log_cb && ctx->cfg.log.modules[module] >= level;
}

__attribute__((format(printf, 4, 5))) P_NONNULL void psycho_log_msg(struct p_ctx *ctx, enum psycho_log_module module,
								    enum psycho_log_level level, const char *fmt, ...);

#define LOG_MODULE_LEVEL_ENABLED(ctx, lvl) psycho_log_enabled(ctx, m_log_module, PSYCHO_LOG_LEVEL_##lvl)

#define LOG_MSG(ctx, lvl, ...)                                                                    \
	do {                                                                                      \
		if (LOG_MODULE_LEVEL_ENABLED(ctx, lvl))                                           \
			psycho_log_msg((ctx), m_log_module, PSYCHO_LOG_LEVEL_##lvl, __VA_ARGS__); \
	} while (0)

#define LOG_INFO(ctx, ...)	      LOG_MSG(ctx, INFO, __VA_ARGS__)
#define LOG_WARN(ctx, ...)	      LOG_MSG(ctx, WARN, __VA_ARGS__)
#define LOG_ERR(ctx, ...)	      LOG_MSG(ctx, ERROR, __VA_ARGS__)
#define LOG_DEBUG(ctx, ...)	      LOG_MSG(ctx, DEBUG, __VA_ARGS__)
#define LOG_TRACE(ctx, ...)	      LOG_MSG(ctx, TRACE, __VA_ARGS__)

#define LOG_INFO_UNCHECKED(ctx, ...)  psycho_log_msg((ctx), m_log_module, PSYCHO_LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN_UNCHECKED(ctx, ...)  psycho_log_msg((ctx), m_log_module, PSYCHO_LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERR_UNCHECKED(ctx, ...)   psycho_log_msg((ctx), m_log_module, PSYCHO_LOG_LEVEL_ERR, __VA_ARGS__)
#define LOG_DEBUG_UNCHECKED(ctx, ...) psycho_log_msg((ctx), m_log_module, PSYCHO_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_TRACE_UNCHECKED(ctx, ...) psycho_log_msg((ctx), m_log_module, PSYCHO_LOG_LEVEL_TRACE, __VA_ARGS__)
