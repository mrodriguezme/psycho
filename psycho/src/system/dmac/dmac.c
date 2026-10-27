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

#include <assert.h>

#include "dmac.h"
#include "defs.h"
#include "system/gpu/gpu.h"
#include "debug/log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_DMAC);

// clang-format off

#define CHANNEL_LIST(X)				\
	X(DMAC_CHANNEL_MDECIN, "MDECin"),	\
	X(DMAC_CHANNEL_MDECOUT, "MDECout"),	\
	X(DMAC_CHANNEL_GPU, "GPU"),		\
	X(DMAC_CHANNEL_CDROM, "CDROM"),		\
	X(DMAC_CHANNEL_SPU, "SPU"),		\
	X(DMAC_CHANNEL_PIO, "PIO"),		\
	X(DMAC_CHANNEL_OTC, "OTC")

// clang-format on

static const struct {
	uint32_t (*src)(struct psycho_ctx *ctx);
	void (*dst)(struct psycho_ctx *ctx, uint32_t word);
} channel_cfg[] = {
	// clang-format off

	#if 0
	[DMAC_CHANNEL_GPU] = {
		.src = psycho_gpuread,
		.dst = psycho_g
	}
		#endif

	// clang-format on
};

PSYCHO_NODISCARD static const char *channel_to_str(enum dmac_channel channel)
{
	static const char *const strs[PSYCHO_DMAC_CHANNELS_NUM] = {
#define X(id, name) [id] = name
		CHANNEL_LIST(X)
#undef X
	};

	assert((channel < ARRAY_SIZE(strs)) && (strs[channel]));
	return strs[channel];
}

void psycho_dmac_reset(struct psycho_ctx *ctx)
{
	ctx->dmac.dpcr = 0x07654321;
}

void dmac_channel_madr_write(struct psycho_ctx *ctx, enum dmac_channel channel, uint32_t madr)
{
	madr &= MADR_MASK;

	ctx->dmac.channels[channel].madr = madr;
	LOG_TRACE(ctx, "%s MADR set to 0x%08X", channel_to_str(channel), madr);
}

void dmac_channel_bcr_write(struct psycho_ctx *ctx, enum dmac_channel channel, uint32_t bcr)
{
	ctx->dmac.channels[channel].bcr = bcr;
	LOG_TRACE(ctx, "%s BCR set to 0x%08X", channel_to_str(channel), bcr);
}

void dmac_channel_chcr_write(struct psycho_ctx *ctx, enum dmac_channel channel, uint32_t word)
{
}

void dmac_dpcr_write(struct psycho_ctx *ctx, uint32_t word)
{
}

void dmac_dicr_write(struct psycho_ctx *ctx, uint32_t word)
{
}

void psycho_dmac_channel_dreq_update(struct psycho_ctx *ctx, enum dmac_channel channel, bool asserted)
{
	if (!(ctx->dmac.channels[channel].chcr & CHCR_START_TRANSFER))
		return;
}
