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

#define JR_RA (0x03E00008)

#include "psycho/ctx.h"

void p_bios_trace_init(struct p_ctx *ctx) P_NONNULL;

P_NONNULL P_ALWAYS_INLINE bool p_bios_trace_in_bios_call(u32 pc)
{
	return (pc == 0xA0) || (pc == 0xB0) || (pc == 0xC0);
}

void p_bios_trace_begin(struct p_ctx *ctx, u32 fn, u32 tbl_off) P_NONNULL;

P_NONNULL P_ALWAYS_INLINE bool p_bios_trace_end_of_call(struct p_ctx *ctx,
							u32 instr)
{
	return (instr == JR_RA) && (ctx->bios_trace.stack.top);
}

void p_bios_trace_end(struct p_ctx *ctx, u32 v0) P_NONNULL;
