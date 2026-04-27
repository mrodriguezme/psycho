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

#include "psycho/ctx.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define LOG_MODULE(mod) static const enum psycho_log_module m_log_module = (mod)

#define LOG_MSG(ctx, module, lvl, args...)                            \
	({                                                            \
		struct psycho_ctx *const m_ctx = (ctx);               \
                                                                      \
		if (m_ctx->log.cfg.modules[(module)] >= (lvl))        \
			psycho_log_msg(m_ctx, (module), (lvl), args); \
	})

#define LOG_INFO(ctx, args...) \
	(LOG_MSG((ctx), m_log_module, PSYCHO_LOG_LEVEL_INFO, args))

#define LOG_WARN(ctx, args...) \
	(LOG_MSG((ctx), m_log_module, PSYCHO_LOG_LEVEL_WARN, args))

#define LOG_ERR(ctx, args...) \
	(LOG_MSG((ctx), m_log_module, PSYCHO_LOG_LEVEL_ERR, args))

#define LOG_DBG(ctx, args...) \
	(LOG_MSG((ctx), m_log_module, PSYCHO_LOG_LEVEL_DBG, args))

#define LOG_TRACE(ctx, args...) \
	(LOG_MSG((ctx), m_log_module, PSYCHO_LOG_LEVEL_TRACE, args))

void psycho_log_init(struct psycho_ctx *ctx, const struct psycho_log_cfg *cfg);

void psycho_log_msg(struct psycho_ctx *const ctx,
		    const enum psycho_log_module module,
		    const enum psycho_log_level level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus
