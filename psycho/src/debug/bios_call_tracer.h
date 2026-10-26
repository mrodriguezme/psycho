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

#include "psycho/ctx.h"
#include "lib/lifo.h"
#include "common/util.h"

LIFO_IMPL(psycho_bios_call_tracer_stack, struct psycho_bios_call_tracer_frame, LIFO_OVERFLOW_POLICY_DROP_NEWEST);

constexpr uint32_t JR_RA = 0x03E00008;

void psycho_bios_call_tracer_init(struct psycho_ctx *ctx) PSYCHO_NONNULL;

PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE bool entering_bios_call(uint32_t pc)
{
	return (pc == 0xA0) || (pc == 0xB0) || (pc == 0xC0);
}

PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE bool leaving_bios_call(struct psycho_ctx *ctx, uint32_t ra, uint32_t instr)
{
	if (instr != JR_RA)
		return false;

	struct psycho_bios_call_tracer_frame *frame = psycho_bios_call_tracer_stack_top(&ctx->bios_call_tracer.stack);
	return frame && frame->ra == ra;
}

PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE size_t bios_call_tracer_pc_to_offset(uint32_t pc)
{
	return (pc >> 4) - 0xA;
}

void psycho_bios_call_tracer_begin(struct psycho_ctx *ctx, uint32_t func, uint32_t table_offset) PSYCHO_NONNULL;

void psycho_bios_call_tracer_end(struct psycho_ctx *ctx, uint32_t v0) PSYCHO_NONNULL;
