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

#include <string.h>

#include "cpu.h"
#include "defs.h"

#include "debug/log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_CPU);

void psycho_cpu_reset(struct psycho_ctx *ctx)
{
	memset(&ctx->cpu.base, 0, sizeof(ctx->cpu.base));
	psycho_cpu_pc_set(ctx, RST_VECTOR);
}

PSYCHO_NONNULL void psycho_cpu_pc_set(struct psycho_ctx *ctx, uint32_t pc)
{
	ctx->cpu.base.delay_pc = pc;
	ctx->cpu.base.pc       = pc;
	ctx->cpu.base.next_pc  = pc + sizeof(ctx->cpu.base.instr);
}

PSYCHO_NONNULL void psycho_cpu_irq_mux_assert(struct psycho_ctx *ctx, bool asserted)
{
	if (asserted) {
		ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR] |= SR_IM2;
		LOG_DEBUG(ctx, "irq mux line asserted");
	} else {
		ctx->cpu.base.cop0_cpr[PSYCHO_CPU_COP0_CPR_SR] &= ~SR_IM2;
		LOG_DEBUG(ctx, "irq mux line not asserted");
	}
}
