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

#define LOG_MOD(mod) static const enum p_log_mod m_log_mod = (mod)

#define MOD_LOG_LVL_ACTIVE(ctx, lvl) log_enabled((ctx), m_log_mod, (lvl))

#define LOG_MSG(ctx, lvl, args...)                                \
	({                                                        \
		if (MOD_LOG_LVL_ACTIVE((ctx), (lvl)))             \
			p_log_msg((ctx), m_log_mod, (lvl), args); \
	})

#define LOG_INFO(ctx, args...) LOG_MSG((ctx), P_LOG_INFO, args)
#define LOG_WARN(ctx, args...) LOG_MSG((ctx), P_LOG_WARN, args)
#define LOG_ERR(ctx, args...) LOG_MSG((ctx), P_LOG_ERR, args)
#define LOG_DBG(ctx, args...) LOG_MSG((ctx), P_LOG_DBG, args)
#define LOG_TRACE(ctx, args...) LOG_MSG((ctx), P_LOG_TRACE, args)

#define LOG_TRACE_UNCHECKED(ctx, args...) \
	p_log_msg((ctx), m_log_mod, P_LOG_TRACE, args)

P_NODISCARD P_ALWAYS_INLINE __attribute__((nonnull)) bool
log_enabled(const struct p_ctx *const ctx, const enum p_log_mod mod,
	    const enum p_log_lvl lvl)
{
	return (ctx->cfg.log.log_cb) && ctx->cfg.log.mod[mod] >= lvl;
}

__attribute__((format(printf, 4, 5))) void
p_log_msg(struct p_ctx *const ctx, const enum p_log_mod mod,
	  const enum p_log_lvl lvl, const char *fmt, ...)
	__attribute__((nonnull));
