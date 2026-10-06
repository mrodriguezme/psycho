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

enum {
	JR_RA = 0x03E00008,
};

void psycho_bios_func_tracer_init(struct psycho_ctx *ctx) PSYCHO_NONNULL;

PSYCHO_NONNULL PSYCHO_STATIC_ALWAYS_INLINE bool entering_bios_func(uint32_t pc)
{
	return (pc == 0xA0) || (pc == 0xB0) || (pc == 0xC0);
}

PSYCHO_NONNULL PSYCHO_STATIC_ALWAYS_INLINE bool leaving_bios_func(struct psycho_ctx *ctx, uint32_t instr)
{
	return (instr == JR_RA) && (ctx->bios_func_tracer.stack.top);
}

void psycho_bios_func_tracer_begin(struct psycho_ctx *ctx, uint32_t fn, uint32_t tbl_off) PSYCHO_NONNULL;

void psycho_bios_func_tracer_end(struct psycho_ctx *ctx, uint32_t v0) PSYCHO_NONNULL;
