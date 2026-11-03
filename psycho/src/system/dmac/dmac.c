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

#include "system/sched.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "system/sbus/sbus.h"
#include "dmac.h"
#include "defs.h"
#include "system/gpu/gpu.h"
#include "debug/log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_DMAC);

static const struct {
	uint32_t (*src)(struct psycho_ctx *ctx);
	void (*dst)(struct psycho_ctx *ctx, uint32_t word);
	bool (*dreq_asserted)(struct psycho_ctx *ctx);
	unsigned int clks_per_word;
} channel_cfg[] = {
	// clang-format off

	[PSYCHO_DMAC_CHANNEL_ID_GPU] = {
		.src		= psycho_gpu_gp1_word_read,
		.dst		= psycho_gp0,
		.dreq_asserted	= psycho_gpu_dreq_asserted,
		.clks_per_word	= 1
	},

	[PSYCHO_DMAC_CHANNEL_ID_OTC] = {
		.clks_per_word = 1
	}

	// clang-format on
};

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

PSYCHO_NONNULL static void dma_transfer_ram_to_device(struct psycho_ctx *ctx, struct psycho_dmac_channel *ch)
{
	int step      = (ch->chcr & CHCR_MADR_PER_STEP) ? -4 : +4;
	uint32_t madr = ch->madr;

	while (ctx->dmac.active_channel.transfer_size--) {
		bool valid;
		uint32_t word = psycho_bus_load_word(ctx, madr, &valid);
		channel_cfg[ctx->dmac.active_channel.id].dst(ctx, word);

		madr += step;
	}

	ch->chcr &= ~CHCR_START_TRANSFER;
	LOG_TRACE(ctx, "channel \"%s\" dma done", channel_to_str(ctx->dmac.active_channel.id));
}

PSYCHO_NONNULL
static void dma_transfer_device_to_ram(struct psycho_ctx *ctx, struct psycho_dmac_channel *ch)
{
	uint32_t count = ctx->dmac.active_channel.transfer_size - 1;
	uint32_t addr  = ch->madr & 0x00FFFFFC;

	while (count--) {
		uint32_t next = (addr - 4) & 0x00FFFFFC;

		if (addr < 0x00800000)
			memcpy(&ctx->bus.ram[addr & 0x001FFFFF], &next, sizeof(next));

		addr = next;
	}

	if (addr < 0x00800000) {
		uint32_t end = 0x00FFFFFF;
		memcpy(&ctx->bus.ram[addr & 0x001FFFFF], &end, sizeof(end));
	}

	ch->chcr &= ~(CHCR_START_TRANSFER | CHCR_FORCE_TRANSFER_START_WITHOUT_DREQ);
	LOG_INFO(ctx, "otc transfer done");
}

PSYCHO_NONNULL static void transfer_event(struct psycho_ctx *ctx, void *)
{
	struct psycho_dmac_channel *ch = &ctx->dmac.channels[ctx->dmac.active_channel.id];

	if (ch->chcr & CHCR_TRANSFER_DIR)
		dma_transfer_ram_to_device(ctx, ch);
	else
		dma_transfer_device_to_ram(ctx, ch);
}

[[nodiscard]] PSYCHO_NONNULL static unsigned int transfer_event_ts(struct psycho_ctx *ctx,
								   enum psycho_dmac_channel_id id, uint32_t chcr)
{
	uint32_t bcr		   = ctx->dmac.channels[id].bcr;
	unsigned int clks_per_word = channel_cfg[id].clks_per_word;

	switch (field_get32(CHCR_SYNCMODE_MASK, chcr)) {
	case CHCR_SYNCMODE_BURST:
		return clks_per_word * field_get32(BC_BS_MASK, bcr);

	case CHCR_SYNCMODE_SLICE:
		return clks_per_word * field_get32(BA_MASK, bcr);

	case CHCR_SYNCMODE_LINKED_LIST:
		return field_get32(BC_BS_MASK, bcr);

	default:
		unreachable();
	}
}

[[nodiscard]] static unsigned int determine_transfer_size(uint32_t bcr, uint32_t chcr)
{
	switch (field_get32(CHCR_SYNCMODE_MASK, chcr)) {
	case CHCR_SYNCMODE_BURST:
		return !bcr ? (UINT16_MAX + 1) : field_get32(BC_BS_MASK, bcr);

	case CHCR_SYNCMODE_SLICE:
		return field_get32(BA_MASK, bcr);

	case CHCR_SYNCMODE_LINKED_LIST:
		// Not used
		return 0;

	default:
		unreachable();
	}
}

PSYCHO_NONNULL static void dma_transfer_event_start(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel,
						    uint32_t word)
{
	ctx->dmac.transfer_event.type = PSYCHO_SCHED_EVENT_TYPE_DMA_TRANSFER;
	ctx->dmac.transfer_event.cb   = transfer_event;
	ctx->dmac.transfer_event.ts   = transfer_event_ts(ctx, channel, word);

	psycho_sched_event_add(ctx, &ctx->dmac.transfer_event);
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
	LOG_INFO(ctx, "dma channels enabled: %u", count);
}

[[nodiscard]] static bool channel_enabled(uint32_t dpcr, enum psycho_dmac_channel_id ch)
{
	return dpcr & (1 << (3 + (ch * 4)));
}

void psycho_dmac_reset(struct psycho_ctx *ctx)
{
	ctx->dmac.dpcr = 0x07654321;
	ctx->dmac.channels[PSYCHO_DMAC_CHANNEL_ID_OTC].chcr |= CHCR_OTC_FIXED_BITS;

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

static void dma_transfer_init(struct psycho_ctx *ctx, enum psycho_dmac_channel_id id, uint32_t chcr)
{
	struct psycho_dmac_channel *ch = &ctx->dmac.channels[id];
	const char *str		       = channel_to_str(id);

	ctx->dmac.active_channel.id	       = id;
	ctx->dmac.active_channel.transfer_size = determine_transfer_size(ch->bcr, chcr);

	if (chcr & CHCR_FORCE_TRANSFER_START_WITHOUT_DREQ) {
		LOG_TRACE(ctx, "starting DMA transfer, not waiting for DREQ (channel \"%s\")", str);
		goto start;
	}

	LOG_TRACE(ctx, "DMA transfer latched (channel \"%s\") - waiting for DREQ", str);

	if (unlikely(!channel_cfg[id].dreq_asserted)) {
		LOG_ERR(ctx, "wtf? channel \"%s\" does not have a DREQ line, this transfer will never run", str);
		return;
	}

	if (channel_cfg[id].dreq_asserted(ctx))
		LOG_TRACE(ctx, "starting DMA transfer as DREQ is already active (channel \"%s\")", str);

start:
	dma_transfer_event_start(ctx, id, chcr);
}

void dmac_channel_chcr_write(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, uint32_t word)
{
	if (channel == PSYCHO_DMAC_CHANNEL_ID_OTC) {
		word &= CHCR_OTC_BITS;
		word |= CHCR_OTC_FIXED_BITS;
	} else
		word &= CHCR_BITS;

	struct psycho_dmac_channel *ch = &ctx->dmac.channels[channel];
	const char *str		       = channel_to_str(channel);

	if (unlikely(!bits_became_set(ch->chcr, word, CHCR_START_TRANSFER)))
		goto store;

	if (unlikely(!channel_enabled(ctx->dmac.dpcr, channel))) {
		LOG_WARN(ctx,
			 "transfer requested on channel \"%s\" but not enabled in DPCR - transfer may take effect when "
			 "enabled there",
			 str);
		goto store;
	}

	if (ctx->dmac.transfer_event.valid) {
		LOG_WARN(ctx, "dma transfer ongoing - transfer may take effect later");
		goto store;
	}

	dma_transfer_init(ctx, channel, word);

store:
	LOG_TRACE(ctx, "%s CHCR set to 0x%08X", channel_to_str(channel), word);
	ctx->dmac.channels[channel].chcr = word;
}

void dmac_dpcr_write(struct psycho_ctx *ctx, uint32_t word)
{
	// XXX: What happens when a DMA channel's master enable is disabled while the DMA transfer is running?
	uint32_t prev_dpcr = ctx->dmac.dpcr;
	ctx->dmac.dpcr	   = word;

	update_priorities(ctx);

	// Go through the channels that are enabled, and check if a transfer is pending.
	for (size_t i = 0; i < ctx->dmac.num_channels_enabled; ++i) {
		enum psycho_dmac_channel_id ch = ctx->dmac.channels_by_priority[i];

		if ((!channel_enabled(prev_dpcr, ch)) && (channel_enabled(word, ch)) &&
		    (ctx->dmac.channels[ch].chcr & CHCR_START_TRANSFER))
			// Master enable was not previously not enabled for this channel, but it has been now.
			dma_transfer_init(ctx, ch, ctx->dmac.channels[ch].chcr);
	}

	LOG_INFO(ctx, "OTC %s", word & DPCR_OTC_MASTER_ENABLE ? "enabled" : "disabled");
}

void dmac_dicr_write(struct psycho_ctx *ctx, uint32_t word)
{
}

void psycho_dmac_channel_dreq_update(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, bool asserted)
{
	if (!(ctx->dmac.channels[channel].chcr & CHCR_START_TRANSFER))
		return;

	if (asserted)
		dma_transfer_event_start(ctx, channel, ctx->dmac.channels[channel].chcr);
}
