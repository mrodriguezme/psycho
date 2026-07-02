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
	// clang-format off

	GPU_ADDR_GPUREAD	= 0x1F801810,
	GPU_ADDR_GP0		= 0x1F801810,
	GPU_ADDR_GP1		= 0x1F801814,
	GPU_ADDR_GPUSTAT	= GPU_ADDR_GP1

	// clang-format on
};

enum {
	// clang-format off

	GP0_CMD_MONO_RECT_1X1_OPAQUE	= 0x68,
	GP0_CMD_CPY_RECT_CPU_TO_VRAM	= 0xA0,
	GP0_CMD_CPY_RECT_VRAM_TO_CPU	= 0xC0,

	// clang-format on
};

enum {
	// clang-format off

	GP1_CMD_RST		= 0x00,
	GP1_CMD_GPU_INFO	= 0x10

	// clang-format on
};

void p_gpu_init(struct p_ctx *ctx) __attribute__((nonnull));
void p_gpu_rst(struct p_ctx *const ctx) __attribute__((nonnull));

void p_gpu_gp0(struct p_ctx *ctx, const u32 packet) __attribute__((nonnull));
void p_gpu_gp1(struct p_ctx *ctx, const u32 packet) __attribute__((nonnull));

__attribute__((nonnull)) P_ALWAYS_INLINE void
vram_px_set(struct p_ctx *const ctx, const size_t x, const size_t y,
	    const u16 data)
{
	ctx->gpu.vram[x + (VRAM_WIDTH * y)] = data;
}

__attribute__((nonnull)) P_ALWAYS_INLINE u16
vram_px_get(struct p_ctx *const ctx, const size_t x, const size_t y)
{
	return ctx->gpu.vram[x + (VRAM_WIDTH * y)];
}

P_NODISCARD P_ALWAYS_INLINE u16 color_to_15bit(const u32 px)
{
	const uint r = (px & UINT8_MAX) >> 3;
	const uint g = ((px >> 8) & UINT8_MAX) >> 3;
	const uint b = ((px >> 16) & UINT8_MAX) >> 3;

	return (b << 10) | (g << 5) | r;
}

P_NODISCARD u32 p_gpu_gpuread(struct p_ctx *ctx) __attribute__((nonnull));
