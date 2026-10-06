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
#include "psycho/scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct psycho_ctx;

enum {
	PSYCHO_GPU_VRAM_HEIGHT = 512,
	PSYCHO_GPU_VRAM_WIDTH = 1024
};

typedef uint16_t psycho_gpu_vram[PSYCHO_GPU_VRAM_HEIGHT][PSYCHO_GPU_VRAM_WIDTH];

struct psycho_gpu_vertex {
	int16_t x;
	int16_t y;
	unsigned int color;
};

struct psycho_gpu_render_ops {
	void (*rect)(struct psycho_ctx *ctx, struct psycho_gpu_vertex *v0);
};

struct psycho_gpu {
	struct {
		void (*fn)(struct psycho_ctx *ctx);
		size_t rem_params;
		size_t params;
		uint32_t data[64];
	} init;

	struct {
		size_t x;
		size_t y;
		size_t x_orig;
		size_t x_max;
		unsigned int rem;
	} copy;

	struct psycho_gpu_vertex rect;

	struct psycho_gpu_render_ops render_ops;

	psycho_gpu_vram vram;

	void (*cmd_fn)(struct psycho_ctx *ctx, uint32_t packet);

	struct psycho_scheduler_event vblank_event;

	uint32_t gpustat;
	uint32_t gpuread;
};

#ifdef __cplusplus
}
#endif // __cplusplus
