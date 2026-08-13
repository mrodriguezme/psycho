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

#define GPU_GPUREAD		 (0x1F801810)
#define GPU_GP0			 (0x1F801810)
#define GPU_GP1			 (0x1F801814)
#define GPU_GPUSTAT		 (0x1F801814)

#define GP0_MONO_RECT_1X1_OPAQUE (0x68)
#define GP0_CPY_RECT_CPU_TO_VRAM (0xA0)
#define GP0_CPY_RECT_VRAM_TO_CPU (0xC0)

#define GP1_RST			 (0x00)
#define GP1_GPU_INFO		 (0x10)

void p_gpu_init(struct p_ctx *ctx) P_NONNULL;
void p_gpu_rst(struct p_ctx *ctx) P_NONNULL;

void p_gp0(struct p_ctx *ctx, u32 packet) P_NONNULL;
void p_gp1(struct p_ctx *ctx, u32 packet) P_NONNULL;

P_NONNULL P_ALWAYS_INLINE void vram_px_set(struct p_ctx *ctx, size_t x,
					   size_t y, u16 data)
{
	ctx->gpu.vram[x + (VRAM_WIDTH * y)] = data;
}

P_NONNULL P_ALWAYS_INLINE u16 vram_px_get(struct p_ctx *ctx, size_t x, size_t y)
{
	return ctx->gpu.vram[x + (VRAM_WIDTH * y)];
}

P_NODISCARD P_ALWAYS_INLINE u16 color_to_15bit(u32 px)
{
	const uint r = (px & UINT8_MAX) >> 3;
	const uint g = ((px >> 8) & UINT8_MAX) >> 3;
	const uint b = ((px >> 16) & UINT8_MAX) >> 3;

	return (b << 10) | (g << 5) | r;
}

P_NODISCARD u32 p_gpuread(struct p_ctx *ctx) P_NONNULL;
