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

void psycho_gpu_init(struct psycho_ctx *ctx) PSYCHO_NONNULL;
void psycho_gpu_reset(struct psycho_ctx *ctx) PSYCHO_NONNULL;

void psycho_gpu_gp0(struct psycho_ctx *ctx, uint32_t packet) PSYCHO_NONNULL;
void psycho_gpi_gp1(struct psycho_ctx *ctx, uint32_t packet) PSYCHO_NONNULL;

PSYCHO_NONNULL PSYCHO_STATIC_ALWAYS_INLINE void vram_px_set(struct psycho_ctx *ctx, size_t x, size_t y, uint16_t data)
{
	ctx->gpu.vram[y][x] = data;
}

PSYCHO_NONNULL PSYCHO_STATIC_ALWAYS_INLINE uint16_t vram_px_get(struct psycho_ctx *ctx, size_t x, size_t y)
{
	return ctx->gpu.vram[y][x];
}

PSYCHO_NODISCARD PSYCHO_STATIC_ALWAYS_INLINE uint16_t color_to_15bit(uint32_t px)
{
	const unsigned int r = (px & UINT8_MAX) >> 3;
	const unsigned int g = ((px >> 8) & UINT8_MAX) >> 3;
	const unsigned int b = ((px >> 16) & UINT8_MAX) >> 3;

	return (b << 10) | (g << 5) | r;
}

PSYCHO_NODISCARD uint32_t p_gpuread(struct psycho_ctx *ctx) PSYCHO_NONNULL;
