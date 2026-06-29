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

#include <stddef.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum {
	VRAM_HEIGHT = 512,
	VRAM_WIDTH = 1024,
};

struct p_ctx;

struct p_gpu {
	struct {
		void (*fn)(struct p_ctx *ctx);
		size_t rem_params;
		size_t params;
		u32 data[64];
	} init;

	struct {
		size_t x;
		size_t y;
		size_t x_orig;
		size_t x_max;
		uint rem;
	} copy;

	u16 *vram;
	void (*cmd_fn)(struct p_ctx *ctx, u32 packet);

	u32 gpustat;
	u32 gpuread;
};

#ifdef __cplusplus
}
#endif // __cplusplus
