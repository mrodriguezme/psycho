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
#include <stdlib.h>

#include "dmac.h"
#include "defs.h"
#include "system/gpu/gpu.h"
#include "debug/log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_DMAC);

static const struct {
	uint32_t (*src)(struct psycho_ctx *ctx);
	void (*dst)(struct psycho_ctx *ctx, uint32_t word);
	unsigned int clks_per_word;
} channel_cfg[] = {
	// clang-format off

	#if 0
	[PSYCHO_DMAC_CHANNEL_ID_GPU] = {
		.src		= nullptr,
		.dst		= nullptr,
	}
		#endif

	// clang-format on
};

static unsigned int channel_transfer_rate_cfg[PSYCHO_DMAC_CHANNEL_ID_COUNT];

PSYCHO_NODISCARD static const char *channel_to_str(enum psycho_dmac_channel_id channel)
{
	static const char *const strs[PSYCHO_DMAC_CHANNEL_ID_COUNT] = {
#define X(id, name) [PSYCHO_DMAC_CHANNEL_ID_##id] = name,
		PSYCHO_DMAC_CHANNEL_LIST(X)
#undef X
	};

	assert((channel < ARRAY_SIZE(strs)) && (strs[channel]));
	return strs[channel];
}

static int dma_ch_priority_sort_cmp(const void *a, const void *b)
{
	unsigned int ch_prio_first  = *(const unsigned int *)a;
	unsigned int ch_prio_second = *(const unsigned int *)b;

	return (ch_prio_first > ch_prio_second) - (ch_prio_first < ch_prio_second);
}

PSYCHO_NONNULL static void update_priorities(struct psycho_ctx *ctx)
{
	unsigned int keys[PSYCHO_DMAC_CHANNEL_ID_COUNT];
	unsigned int count = 0;

	for (enum psycho_dmac_channel_id ch = 0; ch < PSYCHO_DMAC_CHANNEL_ID_COUNT; ++ch) {
		unsigned int nibble = (ctx->dmac.dpcr >> (ch * 4)) & 0xF;

		if (!(nibble & 0x8))
			continue;

		keys[count++] = ((nibble & 0x7) << 3) | (PSYCHO_DMAC_CHANNEL_ID_COUNT - 1 - ch);
	}

	qsort(keys, count, sizeof(keys[0]), dma_ch_priority_sort_cmp);

	for (size_t i = 0; i < count; ++i)
		ctx->dmac.channels_by_priority[i] = PSYCHO_DMAC_CHANNEL_ID_COUNT - 1 - (keys[i] & 0x7);

	ctx->dmac.num_channels_enabled = count;

	if (count)
		LOG_INFO(ctx, "dma channels enabled: %u", count);
}

void psycho_dmac_reset(struct psycho_ctx *ctx)
{
	ctx->dmac.dpcr = 0x07654321;
	ctx->dmac.channels[PSYCHO_DMAC_CHANNEL_ID_OTC].chcr |= CHCR_FIXED_BITS;

	update_priorities(ctx);
}

void dmac_channel_madr_write(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, uint32_t madr)
{
	madr &= MADR_MASK;

	ctx->dmac.channels[channel].madr = madr;
	LOG_TRACE(ctx, "%s MADR set to 0x%08X", channel_to_str(channel), madr);
}

void dmac_channel_bcr_write(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, uint32_t bcr)
{
	ctx->dmac.channels[channel].bcr = bcr;
	LOG_TRACE(ctx, "%s BCR set to 0x%08X", channel_to_str(channel), bcr);
}

void dmac_channel_chcr_write(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, uint32_t word)
{
	if (channel == PSYCHO_DMAC_CHANNEL_ID_OTC)
		word &= CHCR_OTC_BITS;
	else
		word &= CHCR_BITS;

	ctx->dmac.channels[channel].chcr = word;
}

void dmac_dpcr_write(struct psycho_ctx *ctx, uint32_t word)
{
	// XXX: What happens when a DMA channel's master enable is disabled while the DMA transfer is running?
	ctx->dmac.dpcr = word;
	update_priorities(ctx);
}

void dmac_dicr_write(struct psycho_ctx *ctx, uint32_t word)
{
}

void psycho_dmac_channel_dreq_update(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, bool asserted)
{
	if (!(ctx->dmac.channels[channel].chcr & CHCR_START_TRANSFER))
		return;
}
