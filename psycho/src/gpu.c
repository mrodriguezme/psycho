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

#define GPUSTAT_RDY_CMD_WORD_BIT	(1 << 26)
#define GPUSTAT_FIFO_DATA_AVAIL_BIT	(1 << 27)

// clang-format on

static struct p_sched_ev ev_vblank;

struct vram_xfer {
	union {
		struct {
			u16 x;
			u16 y;
			u16 w;
			u16 h;
		};
		u32 raw[2];
	};
};

P_NODISCARD static uint xpos_mask_cpy(const u16 xpos)
{
	return xpos & 0x3FF;
}

P_NODISCARD static uint ypos_mask_cpy(const u16 ypos)
{
	return ypos & 0x1FF;
}

P_NODISCARD static uint xsiz_mask_cpy(const u16 xsiz)
{
	return ((xsiz - 1) & 0x3FF) + 1;
}

P_NODISCARD static uint ysiz_mask_cpy(const u16 ysiz)
{
	return ((ysiz - 1) & 0x1FF) + 1;
}

static void on_vblank(struct p_ctx *const ctx)
{
	p_irq_pending(ctx, IRQ_VBLANK);

	ev_vblank.type = P_SCHED_EV_VBLANK;
	ev_vblank.cb = on_vblank;
	ev_vblank.ts = P_CPU_CLKFREQ_HZ / 60;

	p_sched_add(ctx, &ev_vblank);
}

__attribute__((nonnull)) static void vram_set_pixel(struct p_ctx *const ctx,
						    const size_t x,
						    const size_t y,
						    const u16 data)
{
	ctx->gpu.vram[x + (VRAM_WIDTH * y)] = data;
}

__attribute__((nonnull)) static u16
vram_get_pixel(struct p_ctx *const ctx, const size_t x, const size_t y)
{
	return ctx->gpu.vram[x + (VRAM_WIDTH * y)];
}

__attribute__((nonnull)) static void copy_adv(struct p_ctx *const ctx)
{
	ctx->gpu.copy.x++;

	if (ctx->gpu.copy.x >= ctx->gpu.copy.x_max) {
		ctx->gpu.copy.y++;
		ctx->gpu.copy.x = ctx->gpu.copy.x_orig;
	}
}

__attribute__((nonnull)) static void cpy_pixel_to_vram(struct p_ctx *const ctx,
						       const u16 pixel)
{
	vram_set_pixel(ctx, ctx->gpu.copy.x, ctx->gpu.copy.y, pixel);
	copy_adv(ctx);
}

__attribute__((nonnull)) static u16 cpy_pixel_to_cpu(struct p_ctx *const ctx)
{
	const u16 px = vram_get_pixel(ctx, ctx->gpu.copy.x, ctx->gpu.copy.y);
	copy_adv(ctx);

	return px;
}

__attribute__((nonnull)) static void vram_xfer_init(struct p_ctx *const ctx)
{
	struct vram_xfer *params = (struct vram_xfer *)ctx->gpu.init.data;

	params->y = ypos_mask_cpy(params->y);
	params->x = xpos_mask_cpy(params->x);
	params->w = xsiz_mask_cpy(params->w);
	params->h = ysiz_mask_cpy(params->h);

	ctx->gpu.copy.x = params->x;
	ctx->gpu.copy.x_orig = params->x;
	ctx->gpu.copy.x_max = ctx->gpu.copy.x + params->w;

	ctx->gpu.copy.y = params->y;
	ctx->gpu.copy.rem = (params->w * params->h) / sizeof(u16);
}

__attribute__((nonnull)) static void
cpy_rect_cpu_to_vram_exec(struct p_ctx *const ctx, const u32 data)
{
	cpy_pixel_to_vram(ctx, data & UINT16_MAX);
	cpy_pixel_to_vram(ctx, data >> 16);

	ctx->gpu.copy.rem--;

	if (!ctx->gpu.copy.rem) {
		memset(&ctx->gpu.copy, 0, sizeof(ctx->gpu.copy));
		ctx->gpu.gpustat |= GPUSTAT_RDY_CMD_WORD_BIT;

		LOG_TRACE(ctx, "GP0(A0h) complete");
	}
}

__attribute__((nonnull)) static void
cpy_rect_cpu_to_vram_init(struct p_ctx *const ctx)
{
	vram_xfer_init(ctx);
	ctx->gpu.cmd_fn = cpy_rect_cpu_to_vram_exec;
}

__attribute__((nonnull)) static void
cpy_rect_vram_to_cpu_init(struct p_ctx *const ctx)
{
	vram_xfer_init(ctx);
	ctx->gpu.gpustat |= GPUSTAT_FIFO_DATA_AVAIL_BIT;
}

__attribute__((nonnull)) static void gp0(struct p_ctx *const ctx, const u8 cmd,
					 const u32 param)
{
	switch (cmd) {
	case GP0_CMD_CPY_RECT_CPU_TO_VRAM:
		ctx->gpu.init.fn = cpy_rect_cpu_to_vram_init;
		ctx->gpu.init.rem_params = 2;

		ctx->gpu.gpustat &= ~GPUSTAT_RDY_CMD_WORD_BIT;
		return;

	case GP0_CMD_CPY_RECT_VRAM_TO_CPU:
		ctx->gpu.init.fn = cpy_rect_vram_to_cpu_init;
		ctx->gpu.init.rem_params = 2;

		ctx->gpu.gpustat &= ~GPUSTAT_RDY_CMD_WORD_BIT;
		return;

	default:
		LOG_WARN(ctx, "unknown GP0 command 0x%02X", cmd);
		return;
	}
}

__attribute__((nonnull)) static void handle_gpu_info(struct p_ctx *const ctx,
						     const uint data)
{
	switch (data & 0x07) {
	case 0x07:
		ctx->gpu.gpuread = 0x00000002;
		break;

	default:
		asm("nop");
		break;
	}
}

void p_gpu_init(struct p_ctx *const ctx)
{
	ctx->gpu.vram =
		malloc(VRAM_WIDTH * VRAM_HEIGHT * sizeof(*ctx->gpu.vram));
}

void p_gpu_rst(struct p_ctx *const ctx)
{
	memset(&ev_vblank, 0, sizeof(ev_vblank));

	// All PlayStation GPUs default to NTSC, so initialize the VBLANK event
	// treating it as such.

	ev_vblank.type = P_SCHED_EV_VBLANK;
	ev_vblank.cb = on_vblank;
	ev_vblank.ts = P_CPU_CLKFREQ_HZ / 60;

	p_sched_add(ctx, &ev_vblank);
}

void p_gpu_gp0(struct p_ctx *const ctx, const u32 packet)
{
	LOG_TRACE(ctx, "GP0 <- 0x%08X", packet);

	if (ctx->gpu.gpustat & GPUSTAT_RDY_CMD_WORD_BIT) {
		gp0(ctx, packet >> 24, packet & 0x00FFFFFF);
		return;
	}

	if (ctx->gpu.init.fn) {
		ctx->gpu.init.data[ctx->gpu.init.params++] = packet;
		ctx->gpu.init.rem_params--;

		if (!ctx->gpu.init.rem_params) {
			ctx->gpu.init.fn(ctx);
			memset(&ctx->gpu.init, 0, sizeof(ctx->gpu.init));
		}
	} else
		ctx->gpu.cmd_fn(ctx, packet);
}

void p_gpu_gp1(struct p_ctx *const ctx, const u32 packet)
{
	LOG_TRACE(ctx, "GP1 <- 0x%08X", packet);

	const u8 cmd = packet >> 24;

	switch (cmd) {
	case GP1_CMD_RST:
		ctx->gpu.gpustat = 0x14802000;

		LOG_TRACE(ctx, "command reset");
		return;

	case GP1_CMD_GPU_INFO:
		handle_gpu_info(ctx, packet & 0x00FFFFFF);
		return;

	default:
		LOG_WARN(ctx, "unknown GP1 command 0x%02X", cmd);
		return;
	}
}

u32 p_gpu_gpuread(struct p_ctx *const ctx)
{
	if (ctx->gpu.gpustat & GPUSTAT_FIFO_DATA_AVAIL_BIT) {
		const u16 px0 = cpy_pixel_to_cpu(ctx);
		const u16 px1 = cpy_pixel_to_cpu(ctx);
		ctx->gpu.gpuread = ((u32)px0 << 16) | px1;

		ctx->gpu.copy.rem--;

		if (!ctx->gpu.copy.rem) {
			ctx->gpu.gpustat &= ~GPUSTAT_FIFO_DATA_AVAIL_BIT;
			ctx->gpu.gpustat |= GPUSTAT_RDY_CMD_WORD_BIT;

			memset(&ctx->gpu.copy, 0, sizeof(ctx->gpu.copy));
		}
	}
	return ctx->gpu.gpuread;
}
