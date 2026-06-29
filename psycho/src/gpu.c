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

#include <stdlib.h>
#include <string.h>

#include "gpu.h"
#include "intctrl.h"
#include "log.h"
#include "sched.h"

LOG_MOD(P_LOG_GPU);

// clang-format off

#define GPUSTAT_TEXPAGE_X	((1 << 3) | (1 << 2) | (1 << 1) | (1 << 0))
#define GPUSTAT_TEXPAGE_Y	(1 << 4)
#define GPUSTAT_SEMI_TRANS	((1 << 5) | (1 << 6))
#define GPUSTAT_TEXPAGE_COLORS	((1 << 7) | (1 << 8))
#define GPUSTAT_DITHER_ENABLED	(1 << 9)
#define GPUSTAT_INTERLACE_DRAW	(1 << 10)
#define GPUSTAT_MASK_BIT_DRAW	(1 << 11)
#define GPUSTAT_DRAW_NOT_TO	(1 << 12)
#define GPUSTAT_INTERLACE	(1 << 13)
#define GPUSTAT_REVERSEFLAG	(1 << 14)
#define GPUSTAT_DRAW_EVEN_ODD	(UINT32_C(1) << 31)

// clang-format on

static struct p_sched_ev ev_vblank;

static void on_vblank_ev(struct p_ctx *const ctx)
{
	p_irq_pending(ctx, IRQ_VBLANK);

	ev_vblank.type = P_SCHED_EV_VBLANK;
	ev_vblank.cb = on_vblank_ev;
	ev_vblank.ts = P_CPU_CLKFREQ_HZ / 60;

	p_sched_add(ctx, &ev_vblank);
}

__attribute__((nonnull)) static void
cpy_rect_cpu_to_vram(struct p_ctx *const ctx, const u32 packet)
{
}

__attribute__((nonnull)) static void fifo_clr(struct p_ctx *const ctx)
{
	LOG_TRACE(ctx, "command fifo cleared");
}

__attribute__((nonnull)) static void ack_irq(struct p_ctx *const ctx)
{
	ctx->gpu.gpustat |= (1 << 24);
}

__attribute__((nonnull)) static void display_enable(struct p_ctx *const ctx,
						    const bool enabled)
{
}

__attribute__((nonnull)) static void gp0_cmd(struct p_ctx *const ctx,
					     const u8 cmd, const u32 param)
{
	switch (cmd) {
	case GP0_CMD_NOP:
		LOG_WARN(ctx, "GP0 possible NOP command received; ignored");
		return;

	case GP0_CMD_CPY_RECT_CPU_TO_VRAM:
		ctx->gpu.cmd_handler = &cpy_rect_cpu_to_vram;
		//ctx->gpu.gpustat &= ~GPUSTAT_RDY_TO_RECV_CMD_WORD;

		return;

	case GP0_CMD_CLR_CACHE:
		LOG_WARN(ctx, "GP0 cache command received; ignored");
		return;

	default:
		LOG_WARN(ctx, "unknown GP0 command 0x%02X", cmd);
		return;
	}
}

void p_gpu_init(struct p_ctx *const ctx)
{
	ctx->gpu.vram = malloc(VRAM_NUM_LINES * VRAM_LINE_SIZE);
}

void p_gpu_rst(struct p_ctx *const ctx)
{
	memset(&ev_vblank, 0, sizeof(ev_vblank));

	// All PlayStation GPUs default to NTSC, so initialize the VBLANK event
	// treating it as such.

	ev_vblank.type = P_SCHED_EV_VBLANK;
	ev_vblank.cb = on_vblank_ev;
	ev_vblank.ts = P_CPU_CLKFREQ_HZ / 60;

	p_sched_add(ctx, &ev_vblank);
}

void p_gpu_gp0(struct p_ctx *const ctx, const u32 packet)
{
#if 0
	if (ctx->gpu.gpustat & GPUSTAT_RDY_TO_RECV_CMD_WORD_BIT) {
		gp0_cmd(ctx, packet >> 24, packet & 0x00FFFFFF);
		return;
	}
#endif

	if (!ctx->gpu.cmd_handler) {
		LOG_WARN(ctx, "GP0 received 0x%08X but is busy; ignoring",
			 packet);
		return;
	}

	ctx->gpu.cmd_handler(ctx, packet);
}

void p_gpu_gp1(struct p_ctx *const ctx, const u32 packet)
{
	const u8 cmd = packet >> 24;

	switch (cmd) {
	case GP1_CMD_RST:
		ctx->gpu.gpustat = 0x14802000;

		LOG_TRACE(ctx, "command reset");
		return;

	case GP1_CMD_FIFO_CLR:
		LOG_TRACE(ctx, "command fifo cleared");
		return;

	default:
		LOG_WARN(ctx, "unknown GP1 command 0x%02X", cmd);
		return;
	}
}
