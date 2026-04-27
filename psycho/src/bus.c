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

#include <assert.h>
#include <string.h>

#include "bus.h"
#include "log.h"

LOG_MODULE(PSYCHO_LOG_MODULE_BUS);

enum {
	// clang-format off

	BIOS_PADDR_BEGIN	= 0x1FC00000,
	BIOS_PADDR_END		= 0x1FC7FFFF,
	BIOS_PADDR_MASK		= 0x000FFFFF

	// clang-format on
};

uint8_t *psycho_bus_bios_data_get(struct psycho_ctx *const ctx)
{
	return ctx->bus.bios;
}

uint32_t psycho_bus_load_word(struct psycho_ctx *ctx, const uint32_t paddr)
{
	assert(ctx != NULL);

	uint32_t word;

	switch (paddr) {
	case BIOS_PADDR_BEGIN ... BIOS_PADDR_END:
		memcpy(&word, &ctx->bus.bios[paddr & BIOS_PADDR_MASK],
		       sizeof(uint32_t));
		return word;

	default:
		LOG_WARN(ctx,
			 "unknown word load: 0x%08X; returning 0xFFFF'FFFF");
		return 0xFFFFFFFF;
	}
}
