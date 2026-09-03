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

#include "gpu_sw.h"
#include "gpu.h"
#include "intctrl.h"
#include "log.h"
#include "sched.h"

LOG_MOD(P_LOG_GPU);

#define GPUSTAT_RDY_CMD_WORD_BIT    (1 << 26)
#define GPUSTAT_FIFO_DATA_AVAIL_BIT (1 << 27)

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

P_NODISCARD static uint xpos_mask_cpy(u16 xpos)
{
	return xpos & 0x3FF;
}

P_NODISCARD static uint ypos_mask_cpy(u16 ypos)
{
	return ypos & 0x1FF;
}

P_NODISCARD static uint xsiz_mask_cpy(u16 xsiz)
{
	return ((xsiz - 1) & 0x3FF) + 1;
}

P_NODISCARD static uint ysiz_mask_cpy(u16 ysiz)
{
	return ((ysiz - 1) & 0x1FF) + 1;
}

P_NONNULL static void on_vblank(struct p_ctx *ctx)
{
	if (ctx->cfg.on_vblank)
		ctx->cfg.on_vblank(ctx);

	p_irq_pend(ctx, IRQ_VBLANK);
}

P_NONNULL static void copy_adv(struct p_ctx *ctx)
{
	ctx->gpu.copy.x++;

	if (ctx->gpu.copy.x >= ctx->gpu.copy.x_max) {
		ctx->gpu.copy.y++;
		ctx->gpu.copy.x = ctx->gpu.copy.x_orig;
	}
}

P_NONNULL static void cpy_pixel_to_vram(struct p_ctx *ctx, u16 px)
{
	vram_px_set(ctx, ctx->gpu.copy.x, ctx->gpu.copy.y, px);
	copy_adv(ctx);
}

P_NONNULL static u16 cpy_pixel_to_cpu(struct p_ctx *ctx)
{
	const u16 px = vram_px_get(ctx, ctx->gpu.copy.x, ctx->gpu.copy.y);
	copy_adv(ctx);

	return px;
}

P_NONNULL static void vram_xfer_init(struct p_ctx *ctx)
{
	struct vram_xfer *params = (struct vram_xfer *)ctx->gpu.init.data;

	params->y = ypos_mask_cpy(params->y);
	params->x = xpos_mask_cpy(params->x);
	params->w = xsiz_mask_cpy(params->w);
	params->h = ysiz_mask_cpy(params->h);

	ctx->gpu.copy.x	     = params->x;
	ctx->gpu.copy.x_orig = params->x;
	ctx->gpu.copy.x_max  = ctx->gpu.copy.x + params->w;

	ctx->gpu.copy.y	  = params->y;
	ctx->gpu.copy.rem = (params->w * params->h) / sizeof(u16);
}

P_NONNULL static void cpy_rect_cpu_to_vram_exec(struct p_ctx *ctx, u32 data)
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

P_NONNULL static void draw_rect_init(struct p_ctx *ctx)
{
	ctx->gpu.rect.x = ctx->gpu.init.data[0] & UINT16_MAX;
	ctx->gpu.rect.y = ctx->gpu.init.data[0] >> 16;

	ctx->gpu.render_ops.rect(ctx, &ctx->gpu.rect);
	ctx->gpu.gpustat |= GPUSTAT_RDY_CMD_WORD_BIT;
}

P_NONNULL static void cpy_rect_cpu_to_vram_init(struct p_ctx *ctx)
{
	vram_xfer_init(ctx);
	ctx->gpu.cmd_fn = cpy_rect_cpu_to_vram_exec;
}

P_NONNULL static void cpy_rect_vram_to_cpu_init(struct p_ctx *ctx)
{
	vram_xfer_init(ctx);
	ctx->gpu.gpustat |= GPUSTAT_FIFO_DATA_AVAIL_BIT;
}

P_NONNULL static void gp0(struct p_ctx *ctx, u8 cmd, u32 param)
{
	switch (cmd) {
	case GP0_MONO_RECT_1X1_OPAQUE:
		ctx->gpu.init.fn	 = draw_rect_init;
		ctx->gpu.init.rem_params = 1;
		ctx->gpu.rect.color	 = param;

		ctx->gpu.gpustat &= ~GPUSTAT_RDY_CMD_WORD_BIT;
		return;

	case GP0_CPY_RECT_CPU_TO_VRAM:
		ctx->gpu.init.fn	 = cpy_rect_cpu_to_vram_init;
		ctx->gpu.init.rem_params = 2;

		ctx->gpu.gpustat &= ~GPUSTAT_RDY_CMD_WORD_BIT;
		return;

	case GP0_CPY_RECT_VRAM_TO_CPU:
		ctx->gpu.init.fn	 = cpy_rect_vram_to_cpu_init;
		ctx->gpu.init.rem_params = 2;

		ctx->gpu.gpustat &= ~GPUSTAT_RDY_CMD_WORD_BIT;
		return;

	default:
		LOG_WARN(ctx, "unknown GP0 command 0x%02X", cmd);
		return;
	}
}

P_NONNULL static void handle_gpu_info(struct p_ctx *ctx, uint data)
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

void p_gpu_init(struct p_ctx *ctx)
{
	ctx->gpu.render_ops.rect = p_gpu_sw_draw_rect;
}

void p_gpu_rst(struct p_ctx *ctx)
{
	ctx->gpu.ev_vblank.type	     = P_SCHED_EV_VBLANK;
	ctx->gpu.ev_vblank.cb	     = on_vblank;
	ctx->gpu.ev_vblank.ts	     = P_CPU_CLKFREQ_HZ / 60;
	ctx->gpu.ev_vblank.permanent = true;

	p_sched_add(ctx, &ctx->gpu.ev_vblank);
}

void p_gp0(struct p_ctx *ctx, u32 packet)
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

void p_gp1(struct p_ctx *ctx, u32 packet)
{
	LOG_TRACE(ctx, "GP1 <- 0x%08X", packet);

	const u8 cmd = packet >> 24;

	switch (cmd) {
	case GP1_RST:
		ctx->gpu.gpustat = 0x14802000;

		LOG_TRACE(ctx, "command reset");
		return;

	case GP1_GPU_INFO:
		handle_gpu_info(ctx, packet & 0x00FFFFFF);
		return;

	default:
		LOG_WARN(ctx, "unknown GP1 command 0x%02X", cmd);
		return;
	}
}

u32 p_gpuread(struct p_ctx *ctx)
{
	if (ctx->gpu.gpustat & GPUSTAT_FIFO_DATA_AVAIL_BIT) {
		const u16 px0	 = cpy_pixel_to_cpu(ctx);
		const u16 px1	 = cpy_pixel_to_cpu(ctx);
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
