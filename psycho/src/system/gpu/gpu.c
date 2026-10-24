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

#include "common/util.h"
#include "system/sched.h"
#include "debug/log.h"

#include "backend/software/software.h"
#include "psycho/system/gpu/gpu.h"
#include "defs.h"

#include "system/intc/intc.h"

#include "lib/ring_buffer.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_GPU);

struct vram_xfer {
	uint32_t pixels_remaining;
	uint16_t x0;
	uint16_t y0;
	uint16_t width;
	uint16_t col;
	uint16_t row;
};

RING_BUFFER_IMPL(psycho_gpu_fifo_write, uint32_t, PSYCHO_GPU_FIFO_WRITE_SIZE, RING_BUFFER_OVERFLOW_POLICY_DROP_OLDEST);
RING_BUFFER_IMPL(psycho_gpu_fifo_read, uint32_t, PSYCHO_GPU_FIFO_READ_SIZE, RING_BUFFER_OVERFLOW_POLICY_DROP_OLDEST);

PSYCHO_NONNULL static void irq_ack(struct psycho_ctx *ctx)
{
	ctx->gpu.base.stat &= ~STAT_IRQ;
}

PSYCHO_NONNULL static void display_disabled_set(struct psycho_ctx *ctx, bool disabled)
{
	if (disabled) {
		ctx->gpu.base.stat |= STAT_DMSK;
		LOG_INFO(ctx, "display disabled");
	} else {
		ctx->gpu.base.stat &= ~STAT_DMSK;
		LOG_INFO(ctx, "display enabled");
	}
}

PSYCHO_NODISCARD static int16_t sign_extend_11_16(uint32_t v)
{
	return (int16_t)((int32_t)(v << 21) >> 21);
}

PSYCHO_NONNULL static void dma_direction_set(struct psycho_ctx *ctx, enum dma_direction dir)
{
	field_set32(ctx->gpu.base.stat, STAT_DMD_MASK, dir);
}

PSYCHO_NONNULL static void display_area_start_set(struct psycho_ctx *ctx, uint32_t val)
{
	ctx->gpu.base.display_area_start = val & DISPLAY_AREA_START_MASK;

	uint32_t x = field_get32(DISPLAY_AREA_START_X_MASK, val);
	uint32_t y = field_get32(DISPLAY_AREA_START_Y_MASK, val);

	LOG_INFO(ctx, "display area set to %u x %u", x, y);
}

PSYCHO_NONNULL static void display_range_horizontal_set(struct psycho_ctx *ctx, uint32_t val)
{
	ctx->gpu.base.display_range_horizontal = val & DISPLAY_RANGE_HORIZONTAL_MASK;

	uint32_t x1 = field_get32(DISPLAY_RANGE_HORIZONTAL_X1_MASK, val);
	uint32_t x2 = field_get32(DISPLAY_RANGE_HORIZONTAL_X2_MASK, val);

	LOG_INFO(ctx, "horizontal display range set to x1=%u x2=%u", x1, x2);
}

PSYCHO_NONNULL static void display_range_vertical_set(struct psycho_ctx *ctx, uint32_t val)
{
	ctx->gpu.base.display_range_vertical = val & DISPLAY_RANGE_VERTICAL_MASK;

	uint32_t y1 = field_get32(DISPLAY_RANGE_VERTICAL_Y1_MASK, val);
	uint32_t y2 = field_get32(DISPLAY_RANGE_VERTICAL_Y2_MASK, val);

	LOG_INFO(ctx, "vertical display range set to y1=%u y2=%u", y1, y2);
}

PSYCHO_NODISCARD static uint32_t mode_flag(uint32_t val, uint32_t mode_bit, uint32_t stat_bit)
{
	return (val & mode_bit) ? stat_bit : 0;
}

PSYCHO_NODISCARD static unsigned int display_hres(uint32_t stat)
{
	static const unsigned int hres[4] = { 256, 320, 512, 640 };

	if (stat & STAT_HDS2)
		return 368;

	return hres[field_get32(STAT_HDS_MASK, stat)];
}

PSYCHO_NONNULL static void display_mode_log_changes(struct psycho_ctx *ctx, uint32_t old, uint32_t new)
{
	uint32_t changed = (old ^ new) & STAT_DISPLAY_MODE_MASK;

	if (!changed) {
		LOG_DEBUG(ctx, "display mode set: no change");
		return;
	}

	if (changed & (STAT_HDS_MASK | STAT_HDS2))
		LOG_INFO(ctx, "display mode: horizontal res %u -> %u", display_hres(old), display_hres(new));

	if (changed & STAT_VDS)
		LOG_INFO(ctx, "display mode: vertical res %d -> %d", (old & STAT_VDS) ? 480 : 240,
			 (new & STAT_VDS) ? 480 : 240);

	if (changed & STAT_NPB)
		LOG_INFO(ctx, "display mode: video mode %s -> %s", (old & STAT_NPB) ? "PAL" : "NTSC",
			 (new & STAT_NPB) ? "PAL" : "NTSC");

	if (changed & STAT_LBS)
		LOG_INFO(ctx, "display mode: color depth %d-bit -> %d-bit", (old & STAT_LBS) ? 24 : 15,
			 (new & STAT_LBS) ? 24 : 15);

	if (changed & STAT_IRS)
		LOG_INFO(ctx, "display mode: interlace %s -> %s", (old & STAT_IRS) ? "on" : "off",
			 (new & STAT_IRS) ? "on" : "off");

	if (changed & STAT_REV)
		LOG_INFO(ctx, "display mode: reverseflag %d -> %d", !!(old & STAT_REV), !!(new & STAT_REV));
}

PSYCHO_NONNULL static void display_mode_set(struct psycho_ctx *ctx, uint32_t val)
{
	uint32_t old = ctx->gpu.base.stat;
	uint32_t new = old & ~STAT_DISPLAY_MODE_MASK;

	field_set32(new, STAT_HDS_MASK, field_get32(DISPLAY_MODE_HDS_MASK, val));
	new |= mode_flag(val, DISPLAY_MODE_VDS, STAT_VDS);
	new |= mode_flag(val, DISPLAY_MODE_NPB, STAT_NPB);
	new |= mode_flag(val, DISPLAY_MODE_LBS, STAT_LBS);
	new |= mode_flag(val, DISPLAY_MODE_IRS, STAT_IRS);
	new |= mode_flag(val, DISPLAY_MODE_HDS2, STAT_HDS2);
	new |= mode_flag(val, DISPLAY_MODE_REV, STAT_REV);

	if (!(val & DISPLAY_MODE_IRS))
		new |= STAT_ODE2;

	ctx->gpu.base.stat = new;

	if (LOG_MODULE_LEVEL_ENABLED(ctx, INFO))
		display_mode_log_changes(ctx, old, new);
}

PSYCHO_NONNULL static unsigned int xpos_fill_mask(uint32_t xpos)
{
	return xpos & 0x3F0;
}

PSYCHO_NONNULL static unsigned int xsiz_fill_mask(uint32_t xsiz)
{
	return ((xsiz & 0x3FF) + 0x0F) & ~0x0F;
}

PSYCHO_NONNULL static void on_vblank(struct psycho_ctx *ctx, void *userdata)
{
	(void)userdata;

	if (likely(ctx->cfg.gpu.on_vblank))
		ctx->cfg.gpu.on_vblank(ctx, ctx->cfg.gpu.vblank_userdata);

	psycho_intc_irqs_pending_set(ctx, IRQ_VBLANK);
}

PSYCHO_NONNULL static void write_fifo_push(struct psycho_ctx *ctx, uint32_t data)
{
	if (unlikely(psycho_gpu_fifo_write_full(&ctx->gpu.base.fifo_write)))
		LOG_WARN(ctx, "write fifo full - overwriting oldest entry");

	psycho_gpu_fifo_write_push(&ctx->gpu.base.fifo_write, data);
}

PSYCHO_NONNULL static void draw_polygon(struct psycho_ctx *ctx)
{
	struct psycho_gpu_poly_info poly = {};

	uint32_t cmd = psycho_gpu_fifo_write_pop(&ctx->gpu.base.fifo_write);

	poly.nverts	 = (cmd & POLY_VTX) ? 4 : 3;
	poly.gouraud	 = cmd & POLY_IIP;
	poly.textured	 = cmd & POLY_TME;
	poly.raw_texture = cmd & POLY_TGE;
	poly.semi_trans	 = cmd & POLY_ABE;

	uint32_t color = cmd;

	for (size_t i = 0; i < poly.nverts; ++i) {
		struct psycho_gpu_vertex *vert = &poly.v[i];

		if (i && poly.gouraud)
			color = psycho_gpu_fifo_write_pop(&ctx->gpu.base.fifo_write);

		vert->r = field_get32(POLY_R0_MASK, color);
		vert->g = field_get32(POLY_G0_MASK, color);
		vert->b = field_get32(POLY_B0_MASK, color);

		uint32_t xy = psycho_gpu_fifo_write_pop(&ctx->gpu.base.fifo_write);
		vert->x	    = sign_extend_11_16(xy);
		vert->y	    = sign_extend_11_16(xy >> 16);

		if (!poly.textured)
			continue;

		uint32_t uv = psycho_gpu_fifo_write_pop(&ctx->gpu.base.fifo_write);

		vert->u = uv & UINT8_MAX;
		vert->v = (uv >> 8) & UINT8_MAX;

		if (i == 0)
			poly.clut = uv >> 16;
		else if (i == 1)
			poly.tpage = uv >> 16;
	}

	if (!poly.gouraud) {
		for (size_t i = 1; i < poly.nverts; ++i) {
			poly.v[i].r = poly.v[0].r;
			poly.v[i].g = poly.v[0].g;
			poly.v[i].b = poly.v[0].b;
		}
	}

	// drawing offset bullshit

	ctx->gpu.backend.poly(ctx, &poly);
	ctx->gpu.base.stat |= (STAT_IDLE | STAT_WFEP);
}

PSYCHO_NONNULL static void setup_cmd_poly(struct psycho_ctx *ctx, uint32_t packet)
{
	bool quad	 = packet & POLY_VTX;
	bool gouraud	 = packet & POLY_IIP;
	bool raw_texture = packet & POLY_TGE;
	bool textured	 = packet & POLY_TME;
	bool semi_trans	 = packet & POLY_ABE;

	write_fifo_push(ctx, packet);

	uint32_t verts = quad ? 4 : 3;

	uint32_t words = 1 + verts;
	words += gouraud ? (verts - 1) : 0;
	words += textured ? verts : 0;

	ctx->gpu.base.cmd.handler	 = draw_polygon;
	ctx->gpu.base.cmd.required_words = words;

	LOG_TRACE(ctx, "poly cmd 0x%02X: %s %s%s%s%s", packet >> PACKET_CMD_SHIFT, quad ? "quad" : "triangle",
		  gouraud ? "gouraud" : "flat", textured ? ", textured" : "", raw_texture ? ", raw-texture" : "",
		  semi_trans ? ", semi-transparent" : "");

	LOG_TRACE(ctx, "poly color0=(%u,%u,%u) verts=%u required_words=%u", field_get32(POLY_R0_MASK, packet),
		  field_get32(POLY_G0_MASK, packet), field_get32(POLY_B0_MASK, packet), verts, words);

	// Normally, WFEP gets cleared when the command execution is busy (i.e., when the command and all of its
	// parameters are received), however, for Polygon and Line Rendering commands, the bit gets cleared immediately
	// after receiving the command word (i.e., before receiving the vertex parameters).
	ctx->gpu.base.stat &= ~(STAT_IDLE | STAT_WFEP);
}

PSYCHO_NONNULL static void fill_vram_handler(struct psycho_ctx *ctx)
{
	uint32_t cmd  = psycho_gpu_fifo_write_pop(&ctx->gpu.base.fifo_write);
	uint32_t pos  = psycho_gpu_fifo_write_pop(&ctx->gpu.base.fifo_write);
	uint32_t size = psycho_gpu_fifo_write_pop(&ctx->gpu.base.fifo_write);

	struct psycho_gpu_fill_info info;

	info.x = xpos_fill_mask(pos);
	info.y = (pos >> 16) & 0x1FF;
	info.w = xsiz_fill_mask(size);
	info.h = (size >> 16) & 0x1FF;

	uint8_t r = field_get32(POLY_R0_MASK, cmd);
	uint8_t g = field_get32(POLY_G0_MASK, cmd);
	uint8_t b = field_get32(POLY_B0_MASK, cmd);

	// Low 3 bits of each channel dropped, mask bit (15) forced to 0.
	uint16_t color = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);

	LOG_TRACE(ctx, "quick fill: pos=(%u,%u) size=%ux%u color=0x%04X", info.x, info.y, info.w, info.h, color);

	ctx->gpu.backend.fill(ctx, &info);
	ctx->gpu.base.stat |= (STAT_IDLE | STAT_WFEP);
}

PSYCHO_NONNULL static void fill_vram(struct psycho_ctx *ctx, uint32_t packet)
{
	write_fifo_push(ctx, packet);

	ctx->gpu.base.cmd.handler	 = fill_vram_handler;
	ctx->gpu.base.cmd.required_words = 3;

	ctx->gpu.base.stat &= ~STAT_IDLE;
}

PSYCHO_NONNULL static void setup_cmd_misc(struct psycho_ctx *ctx, uint32_t packet)
{
	uint8_t cmd_type = packet >> PACKET_CMD_SHIFT;

	switch (cmd_type) {
	case MISC_CMD_NOP:
		LOG_INFO(ctx, "nop");
		break;

	case MISC_CMD_FILL_VRAM:
		fill_vram(ctx, packet);
		break;

	default:
		LOG_INFO(ctx, "unknown misc cmd 0x%02X", cmd_type);
		break;
	}
}

PSYCHO_NONNULL static void draw_mode_set(struct psycho_ctx *ctx, uint32_t val)
{
	uint32_t bits = val & STAT_DRAW_MODE_DIRECT_MASK;

	if (val & DRAW_MODE_TBY2)
		bits |= STAT_TBY2;

	ctx->gpu.base.stat = (ctx->gpu.base.stat & ~STAT_DRAW_MODE_MASK) | bits;

	LOG_INFO(ctx, "draw mode set: tpage=(%u,%d) abr=%u tpf=%u dither=%d draw_to_display_area=%d",
		 field_get32(STAT_TBX_MASK, val) * 64, !!(val & STAT_TBY) * 256, field_get32(STAT_ABR_MASK, val),
		 field_get32(STAT_TPF_MASK, val), !!(val & STAT_DTD), !!(val & STAT_DFE));
}

PSYCHO_NONNULL static void drawing_area_top_left_set(struct psycho_ctx *ctx, uint32_t val)
{
	unsigned int x1 = field_get32(DRAWING_AREA_MASK_X1, val);
	unsigned int y1 = field_get32(DRAWING_AREA_MASK_Y1, val);

	ctx->gpu.base.drawing_area_top_left = val & DRAWING_AREA_MASK_TOP_LEFT;
	LOG_INFO(ctx, "top left drawing area set to (%u, %u)", x1, y1);
}

PSYCHO_NONNULL static void drawing_area_bottom_right_set(struct psycho_ctx *ctx, uint32_t val)
{
	unsigned int x2 = field_get32(DRAWING_AREA_MASK_X2, val);
	unsigned int y2 = field_get32(DRAWING_AREA_MASK_Y2, val);

	ctx->gpu.base.drawing_area_bottom_right = val & DRAWING_AREA_MASK_BOTTOM_RIGHT;
	LOG_INFO(ctx, "bottom right drawing area set to (%u, %u)", x2, y2);
}

PSYCHO_NONNULL static void drawing_offset_set(struct psycho_ctx *ctx, uint32_t val)
{
	ctx->gpu.base.drawing_offset_x = sign_extend_11_16(field_get32(DRAWING_OFFSET_X_MASK, val));
	ctx->gpu.base.drawing_offset_y = sign_extend_11_16(field_get32(DRAWING_OFFSET_Y_MASK, val));

	LOG_INFO(ctx, "drawing offset set to (%d, %d)", ctx->gpu.base.drawing_offset_x, ctx->gpu.base.drawing_offset_y);
}

PSYCHO_NONNULL static void mask_bit_set(struct psycho_ctx *ctx, uint32_t val)
{
	uint32_t bits = 0;

	if (val & MASK_BIT_SETTING_PBW)
		bits |= STAT_PBW;

	if (val & MASK_BIT_SETTING_PBC)
		bits |= STAT_PBC;

	ctx->gpu.base.stat = (ctx->gpu.base.stat & ~(STAT_PBW | STAT_PBC)) | bits;
	LOG_INFO(ctx, "mask bit set: force_bit15=%d check_bit15=%d", !!(val & MASK_BIT_SETTING_PBW),
		 !!(val & MASK_BIT_SETTING_PBC));
}

PSYCHO_NONNULL static void do_cmd_env(struct psycho_ctx *ctx, uint32_t packet)
{
	uint8_t cmd_type    = packet >> PACKET_CMD_SHIFT;
	unsigned int params = packet & PACKET_PARAMS_MASK;

	switch (cmd_type) {
	case ENV_CMD_DRAW_MODE_SET:
		draw_mode_set(ctx, params);
		break;

	case ENV_CMD_DRAWING_AREA_TOP_LEFT_SET:
		drawing_area_top_left_set(ctx, params);
		break;

	case ENV_CMD_DRAWING_AREA_BOTTOM_RIGHT_SET:
		drawing_area_bottom_right_set(ctx, params);
		break;

	case ENV_CMD_DRAWING_OFFSET_SET:
		drawing_offset_set(ctx, params);
		break;

	case ENV_CMD_MASK_BIT_SET:
		mask_bit_set(ctx, params);
		break;

	default:
		LOG_INFO(ctx, "unknown env cmd 0x%02X", cmd_type);
		break;
	}
}

PSYCHO_NONNULL static void setup_cmd(struct psycho_ctx *ctx, uint32_t packet)
{
	unsigned int cmd_type = packet >> PACKET_CMD_TYPE_SHIFT;

	switch (cmd_type) {
	case CMD_TYPE_MISC:
		setup_cmd_misc(ctx, packet);
		break;

	case CMD_TYPE_POLY:
		setup_cmd_poly(ctx, packet);
		break;

	case CMD_TYPE_ENV:
		do_cmd_env(ctx, packet);
		break;

	default:
		LOG_WARN(ctx, "unhandled command type %u", cmd_type);
		break;
	}
}

PSYCHO_NONNULL static void abort_cmd(struct psycho_ctx *ctx)
{
	ctx->gpu.gpustat |= STAT_IDLE;
}

void psycho_gpu_reset(struct psycho_ctx *ctx)
{
	ctx->gpu.vblank_event.type	= PSYCHO_SCHED_EVENT_TYPE_VBLANK;
	ctx->gpu.vblank_event.cb	= on_vblank;
	ctx->gpu.vblank_event.ts	= PSYCHO_CPU_CLKFREQ_HZ / 60;
	ctx->gpu.vblank_event.permanent = true;

	ctx->gpu.base.stat = STAT_IDLE | STAT_WFEP;
	psycho_sched_add(ctx, &ctx->gpu.vblank_event);
}

void psycho_gpu_gp0_write_word(struct psycho_ctx *ctx, uint32_t packet)
{
	LOG_TRACE(ctx, "GP0 <- 0x%08X", packet);

	if (ctx->gpu.base.stat & STAT_IDLE) {
		setup_cmd(ctx, packet);
		return;
	}

	write_fifo_push(ctx, packet);

	if (psycho_gpu_write_fifo_size(&ctx->gpu.write_fifo) == ctx->gpu.cmd.required_words)
		ctx->gpu.cmd.handler(ctx);
}

void psycho_gpu_gp1(struct psycho_ctx *ctx, uint32_t packet)
{
	LOG_TRACE(ctx, "GP1 <- 0x%08X", packet);

	uint8_t cmd = packet >> PACKET_CMD_SHIFT;

	switch (cmd) {
	case GP1_CMD_RESET:
		abort_cmd(ctx);
		write_fifo_clear(ctx);
		irq_ack(ctx);
		display_disabled_set(ctx, DISPLAY_DISABLED_RESET_VALUE);
		dma_direction_set(ctx, DMA_DIRECTION_RESET_VALUE);
		display_area_start_set(ctx, DISPLAY_AREA_START_RESET_VALUE);
		display_range_horizontal_set(ctx, DISPLAY_RANGE_HORIZONTAL_RESET_VALUE);
		display_range_vertical_set(ctx, DISPLAY_RANGE_VERTICAL_RESET_VALUE);
		display_mode_set(ctx, DISPLAY_MODE_RESET_VALUE);

		LOG_INFO(ctx, "GPU reset by guest software");
		return;

	case GP1_CMD_DISPLAY_DISABLED_SET:
		display_disabled_set(ctx, packet & 1);
		return;

	case GP1_CMD_DISPLAY_AREA_START_SET:
		display_area_start_set(ctx, packet);
		return;

	case GP1_CMD_DISPLAY_RANGE_HORIZONTAL_SET:
		display_range_horizontal_set(ctx, packet);
		return;

	case GP1_CMD_DISPLAY_RANGE_VERTICAL_SET:
		display_range_vertical_set(ctx, packet);
		return;

	case GP1_CMD_DISPLAY_MODE_SET:
		display_mode_set(ctx, packet);
		return;

	default:
		LOG_WARN(ctx, "unknown GP1 command 0x%02X", cmd);
		return;
	}
}

uint32_t psycho_gpuread(struct psycho_ctx *ctx)
{
	return UINT32_MAX;
}
