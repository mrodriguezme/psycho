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

#include "software.h"

#include "system/gpu/defs.h"
#include "psycho/ctx.h"

static void reset(struct psycho_ctx *ctx)
{
}

PSYCHO_NODISCARD static unsigned int dither_channel(unsigned int c, int d)
{
	return clamp((int)c + d, 0, UINT8_MAX);
}

// Positive when p is on the interior side of edge a->b (for a triangle wound so the total area is positive).
PSYCHO_NODISCARD static int edge(struct psycho_gpu_vertex *a, struct psycho_gpu_vertex *b, int px, int py)
{
	return (b->x - a->x) * (py - a->y) - (b->y - a->y) * (px - a->x);
}

// With y pointing down and positive area, the top edge is horizontal going right and left edges go up.
PSYCHO_NODISCARD static bool is_top_left(struct psycho_gpu_vertex *a, struct psycho_gpu_vertex *b)
{
	int dx = b->x - a->x;
	int dy = b->y - a->y;

	return ((dy == 0) && (dx > 0)) || dy < 0;
}

PSYCHO_NODISCARD static uint16_t rgb888_to_rgb555(unsigned int r, unsigned int g, unsigned int b)
{
	return (uint16_t)((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10));
}

PSYCHO_NONNULL static void triangle(struct psycho_ctx *ctx, struct psycho_gpu_poly_info *poly, unsigned int i0,
				    unsigned int i1, unsigned int i2)
{
	struct psycho_gpu_vertex *v0 = &poly->v[i0];
	struct psycho_gpu_vertex *v1 = &poly->v[i1];
	struct psycho_gpu_vertex *v2 = &poly->v[i2];

	int area = edge(v0, v1, v2->x, v2->y);

	// Degenerate triangle.
	if (area == 0)
		return;

	// The GPU draws both windings, so normalize to positive area by swapping two vertices.
	if (area < 0) {
		swap(v1, v2);
		area = -area;
	}

	// Hardware rejects polygons whose vertices are too far apart.
	int minx = min3(v0->x, v1->x, v2->x);
	int maxx = max3(v0->x, v1->x, v2->x);
	int miny = min3(v0->y, v1->y, v2->y);
	int maxy = max3(v0->y, v1->y, v2->y);

	if (((maxx - minx) > (PSYCHO_GPU_VRAM_WIDTH - 1)) || (((maxy - miny) > PSYCHO_GPU_VRAM_HEIGHT)))
		return;

	// TODO: clamp to the drawing area (GP0 E3h/E4h) instead of all of VRAM.
	minx = clamp(minx, 0, (int)PSYCHO_GPU_VRAM_WIDTH - 1);
	maxx = clamp(maxx, 0, (int)PSYCHO_GPU_VRAM_WIDTH - 1);
	miny = clamp(miny, 0, (int)PSYCHO_GPU_VRAM_HEIGHT - 1);
	maxy = clamp(maxy, 0, (int)PSYCHO_GPU_VRAM_HEIGHT - 1);

	// Pixels exactly on a shared edge belong to only one of the two triangles.
	int bias0 = is_top_left(v1, v2) ? 0 : -1;
	int bias1 = is_top_left(v2, v0) ? 0 : -1;
	int bias2 = is_top_left(v0, v1) ? 0 : -1;

	for (int y = miny; y <= maxy; ++y) {
		for (int x = minx; x <= maxx; ++x) {
			int w0 = edge(v1, v2, x, y);
			int w1 = edge(v2, v0, x, y);
			int w2 = edge(v0, v1, x, y);

			if (((w0 + bias0) < 0) || ((w1 + bias1) < 0) || ((w2 + bias2) < 0))
				continue;

			unsigned int r = ((w0 * v0->r) + (w1 * v1->r) + (w2 * v2->r)) / area;
			unsigned int g = ((w0 * v0->g) + (w1 * v1->g) + (w2 * v2->g)) / area;
			unsigned int b = ((w0 * v0->b) + (w1 * v1->b) + (w2 * v2->b)) / area;

			if (ctx->gpu.base.stat & STAT_DTD) {
				int d = psycho_gpu_dither_matrix[y & 3][x & 3];

				r = dither_channel(r, d);
				g = dither_channel(g, d);
				b = dither_channel(b, d);
			}

			ctx->gpu.software.vram[y][x] = rgb888_to_rgb555(r, g, b);
		}
	}
}

static void poly(struct psycho_ctx *ctx, struct psycho_gpu_poly_info *poly)
{
	if (poly->textured) {
		return;
	}

	triangle(ctx, poly, 0, 1, 2);

	if (poly->nverts == 4)
		triangle(ctx, poly, 1, 2, 3);
}

static void fill(struct psycho_ctx *ctx, struct psycho_gpu_fill_info *info)
{
	for (int row = 0; row < info->h; ++row) {
		uint16_t *line = ctx->gpu.software.vram[(info->y + row) & (PSYCHO_GPU_VRAM_HEIGHT - 1)];

		for (int col = 0; col < info->w; ++col)
			line[(info->x + col) & (PSYCHO_GPU_VRAM_WIDTH - 1)] = info->color;
	}
}

static void rect(struct psycho_ctx *ctx, struct psycho_gpu_rect_info *info)
{
}

const struct psycho_gpu_backend_ops psycho_gpu_backend_ops_software = {
	// clang-format off

	.reset	= reset,
	.name	= "Software",
	.fill	= fill,
	.poly	= poly,
	.rect	= rect,

	// clang-format on
};
