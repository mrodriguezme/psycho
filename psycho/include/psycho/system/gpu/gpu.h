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

#include <stddef.h>

#include "psycho/system/sched.h"
#include "psycho/lib/ring_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct psycho_ctx;

constexpr uint32_t PSYCHO_GPU_VRAM_HEIGHT     = 512;
constexpr uint32_t PSYCHO_GPU_VRAM_WIDTH      = 1024;
constexpr uint32_t PSYCHO_GPU_FIFO_WRITE_SIZE = 16;
constexpr uint32_t PSYCHO_GPU_FIFO_READ_SIZE  = 16;

typedef uint16_t psycho_gpu_vram[PSYCHO_GPU_VRAM_HEIGHT][PSYCHO_GPU_VRAM_WIDTH];

PSYCHO_RING_BUFFER_DEFINE(psycho_gpu_fifo_write, uint32_t, PSYCHO_GPU_FIFO_WRITE_SIZE);
PSYCHO_RING_BUFFER_DEFINE(psycho_gpu_fifo_read, uint32_t, PSYCHO_GPU_FIFO_READ_SIZE);

enum psycho_gpu_backend {
	PSYCHO_GPU_BACKEND_SOFTWARE,
	PSYCHO_GPU_BACKEND_COUNT,
};

struct psycho_gpu_cfg {
	enum psycho_gpu_backend backend;
	void (*on_vblank)(struct psycho_ctx *ctx, void *userdata);
	void *vblank_userdata;
};

struct psycho_gpu {
	struct psycho_gpu_fifo_write fifo_write;
	struct psycho_gpu_fifo_read fifo_read;

	struct psycho_sched_event vblank_event;

	struct {
		void (*handler)(struct psycho_ctx *ctx);
		size_t required_words;
	} cmd;

	uint32_t stat;
	uint32_t gpuread;
	uint32_t display_area_start;
	uint32_t display_range_horizontal;
	uint32_t display_range_vertical;
	uint32_t drawing_area_top_left;
	uint32_t drawing_area_bottom_right;

	int16_t drawing_offset_x;
	int16_t drawing_offset_y;
};

#ifdef __cplusplus
}
#endif // __cplusplus
