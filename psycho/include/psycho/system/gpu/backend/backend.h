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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct psycho_ctx;

struct psycho_gpu_vertex {
	int16_t x, y;
	uint8_t r, g, b;
	uint8_t u, v;
};

struct psycho_gpu_rect_info {

};

struct psycho_gpu_poly_info {
	struct psycho_gpu_vertex v[4];
	unsigned int nverts;
	uint16_t clut;
	uint16_t tpage;
	bool gouraud;
	bool textured;
	bool raw_texture;
	bool semi_trans;
};

struct psycho_gpu_fill_info {
	uint32_t color;
	int16_t x;
	int16_t y;
	int16_t w;
	int16_t h;
};

struct psycho_gpu_backend_ops {
	const char *name;
	void (*rect)(struct psycho_ctx *ctx, struct psycho_gpu_rect_info *rect);
	void (*poly)(struct psycho_ctx *ctx, struct psycho_gpu_poly_info *poly);
	void (*fill)(struct psycho_ctx *ctx, struct psycho_gpu_fill_info *fill);
};

#ifdef __cplusplus
}
#endif // __cplusplus
