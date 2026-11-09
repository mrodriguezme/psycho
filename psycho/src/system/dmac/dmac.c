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
#include <string.h>

#include "system/sbus/sbus.h"
#include "system/sched.h"
#include "system/gpu/gpu.h"
#include "system/intc/intc.h"

#include "dmac.h"
#include "defs.h"

#include "debug/log.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_DMAC);

static void gpu_transfer_burst_mode(struct psycho_ctx *ctx);
static void gpu_transfer_slice_mode(struct psycho_ctx *ctx);
static void gpu_transfer_linked_list_mode(struct psycho_ctx *ctx);
static void otc_transfer_burst_mode(struct psycho_ctx *ctx);

enum dicr_event {
	DICR_EVENT_TRANSFER_COMPLETE,
	DICR_EVENT_SLICE_OR_NODE,
};

static struct channel_route {
	void (*mode_handlers[4])(struct psycho_ctx *ctx);
	uint32_t (*src)(struct psycho_ctx *ctx);
	void (*dst)(struct psycho_ctx *ctx, uint32_t word);
	bool (*dreq_line)(struct psycho_ctx *ctx);
	unsigned int clks_per_word;
} const route_cfg[] = {
	// clang-format off

	[PSYCHO_DMAC_CHANNEL_ID_GPU] = {
		.mode_handlers	= {
			[CHCR_SYNCMODE_BURST]		= gpu_transfer_burst_mode,
			[CHCR_SYNCMODE_SLICE]		= gpu_transfer_slice_mode,
			[CHCR_SYNCMODE_LINKED_LIST]	= gpu_transfer_linked_list_mode,
		},

		.src		= psycho_gpu_gp1_word_read,
		.dst		= psycho_gpu_gp0_word_write,
		.dreq_line	= psycho_gpu_dreq_line,
		.clks_per_word	= 1
	},

	[PSYCHO_DMAC_CHANNEL_ID_OTC] = {
		.mode_handlers		= {
			[CHCR_SYNCMODE_BURST]		= otc_transfer_burst_mode,
			[CHCR_SYNCMODE_SLICE]		= nullptr,
			[CHCR_SYNCMODE_LINKED_LIST]	= nullptr
		},

		.src		= nullptr,
		.dst		= nullptr,
		.dreq_line	= nullptr,
		.clks_per_word	= 1
	}

	// clang-format on
};

[[nodiscard]] PSYCHO_PURE static const char *channel_to_str(enum psycho_dmac_channel_id channel)
{
	static const char *const strs[PSYCHO_DMAC_CHANNEL_ID_COUNT] = {
#define X(id, name) [PSYCHO_DMAC_CHANNEL_ID_##id] = name,
		PSYCHO_DMAC_CHANNEL_LIST(X)
#undef X
	};

	assert((channel < ARRAY_SIZE(strs)) && (strs[channel]));
	return strs[channel];
}

[[nodiscard]] static uint32_t burst_word_count(uint32_t bcr)
{
	uint32_t bs = field_get32(BC_BS_MASK, bcr);
	return bs ? bs : (UINT16_MAX + 1);
}

[[nodiscard]] static bool dicr_irq_active(uint32_t dicr)
{
	return (dicr & DICR_BUS_ERR_FLAG) ||
	       ((dicr & DICR_MASTER_INTERRUPT_ENABLE) && (dicr & DICR_CHANNEL_INTERRUPT_FLAGS));
}

PSYCHO_NONNULL static void dicr_update(struct psycho_ctx *ctx, bool prev_master)
{
	if (dicr_irq_active(ctx->dmac.dicr))
		ctx->dmac.dicr |= DICR_MASTER_INTERRUPT_FLAG;
	else
		ctx->dmac.dicr &= ~DICR_MASTER_INTERRUPT_FLAG;

	if (!prev_master && (ctx->dmac.dicr & DICR_MASTER_INTERRUPT_FLAG))
		psycho_intc_irqs_pending_set(ctx, IRQ_DMAC);
}

PSYCHO_NONNULL static void raise_interrupt(struct psycho_ctx *ctx, enum psycho_dmac_channel_id ch, enum dicr_event ev)
{
	uint32_t *dicr = &ctx->dmac.dicr;

	bool prev = *dicr & DICR_MASTER_INTERRUPT_FLAG;

	uint32_t every_slice = 1 << ch;
	uint32_t ch_enable   = 1 << (16 + ch);

	bool eligible = (ev == DICR_EVENT_TRANSFER_COMPLETE) || (*dicr & every_slice);

	if (eligible && (*dicr & ch_enable) && (*dicr & DICR_MASTER_INTERRUPT_ENABLE))
		*dicr |= 1 << (24 + ch);

	dicr_update(ctx, prev);
}

PSYCHO_NONNULL static void dma_done(struct psycho_ctx *ctx, struct psycho_dmac_channel *ch,
				    enum psycho_dmac_channel_id id)
{
	ch->chcr &= ~(CHCR_START_TRANSFER | CHCR_FORCE_TRANSFER_START_WITHOUT_DREQ);
	LOG_TRACE(ctx, "channel \"%s\" dma done", channel_to_str(id));

	raise_interrupt(ctx, id, DICR_EVENT_TRANSFER_COMPLETE);

	// Fire the next event off
}

PSYCHO_NONNULL static void transfer_ram_to_device(struct psycho_ctx *ctx, enum psycho_dmac_channel_id ch,
						  uint32_t *madr, size_t size, int step)
{
	while (size--) {
		bool valid;
		uint32_t word = psycho_bus_load_word(ctx, *madr, &valid);

		if (unlikely(!valid)) {
			ctx->dmac.dicr |= DICR_BUS_ERR_FLAG;
			return;
		}

		route_cfg[ch].dst(ctx, word);
		*madr += step;
	}
}

PSYCHO_NONNULL static void transfer_device_to_ram(struct psycho_ctx *ctx, enum psycho_dmac_channel_id ch,
						  uint32_t *madr, size_t size, int step)
{
	while (size--) {
		uint32_t word = route_cfg[ch].src(ctx);

		if (unlikely(!psycho_bus_store_word(ctx, *madr, word))) {
			ctx->dmac.dicr |= DICR_BUS_ERR_FLAG;
			return;
		}

		*madr += step;
	}
}

PSYCHO_NONNULL static void gpu_transfer_burst_mode(struct psycho_ctx *ctx)
{
	struct psycho_dmac_channel *ch = &ctx->dmac.channels[PSYCHO_DMAC_CHANNEL_ID_GPU];

	int step = (ch->chcr & CHCR_MADR_PER_STEP) ? -4 : +4;

	uint32_t madr = ch->madr;
	uint32_t size = burst_word_count(ch->bcr);

	if (ch->chcr & CHCR_TRANSFER_DIR)
		transfer_ram_to_device(ctx, PSYCHO_DMAC_CHANNEL_ID_GPU, &madr, size, step);
	else
		transfer_device_to_ram(ctx, PSYCHO_DMAC_CHANNEL_ID_GPU, &madr, size, step);

	dma_done(ctx, ch, PSYCHO_DMAC_CHANNEL_ID_GPU);
}

PSYCHO_NONNULL static void gpu_transfer_slice_mode(struct psycho_ctx *ctx)
{
	struct psycho_dmac_channel *ch = &ctx->dmac.channels[PSYCHO_DMAC_CHANNEL_ID_GPU];
	int step		       = (ch->chcr & CHCR_MADR_PER_STEP) ? -4 : +4;

	uint16_t bs = field_get32(BC_BS_MASK, ch->bcr);
	uint16_t ba = field_get32(BA_MASK, ch->bcr);

	while (ba) {
		if (ch->chcr & CHCR_TRANSFER_DIR)
			transfer_ram_to_device(ctx, PSYCHO_DMAC_CHANNEL_ID_GPU, &ch->madr, bs, step);
		else
			transfer_device_to_ram(ctx, PSYCHO_DMAC_CHANNEL_ID_GPU, &ch->madr, bs, step);

		ba--;

		if (!ba)
			break;

		raise_interrupt(ctx, PSYCHO_DMAC_CHANNEL_ID_GPU, DICR_EVENT_SLICE_OR_NODE);

		if (!route_cfg[PSYCHO_DMAC_CHANNEL_ID_GPU].dreq_line(ctx))
			break;
	}

	ch->bcr = field_prep32(BC_BS_MASK, bs) | field_prep32(BA_MASK, ba);

	if (!ba)
		dma_done(ctx, ch, PSYCHO_DMAC_CHANNEL_ID_GPU);
}

PSYCHO_NONNULL static void gpu_transfer_linked_list_mode(struct psycho_ctx *ctx)
{
	//__builtin_trap();
}

PSYCHO_NONNULL static void otc_ram_write(struct psycho_ctx *ctx, uint32_t addr, uint32_t word)
{
	if (addr < 0x00800000)
		memcpy(&ctx->bus.ram[addr & 0x001FFFFC], &word, sizeof(word));
}

PSYCHO_NONNULL static void otc_transfer_burst_mode(struct psycho_ctx *ctx)
{
	struct psycho_dmac_channel *ch = &ctx->dmac.channels[PSYCHO_DMAC_CHANNEL_ID_OTC];

	uint32_t size = burst_word_count(ch->bcr) - 1;
	uint32_t addr = ch->madr & 0x00FFFFFC;

	while (size--) {
		uint32_t next = (addr - 4) & 0x00FFFFFC;

		otc_ram_write(ctx, addr, next);
		addr = next;
	}

	otc_ram_write(ctx, addr, 0x00FFFFFF);

	dma_done(ctx, ch, PSYCHO_DMAC_CHANNEL_ID_OTC);
}

[[nodiscard]] PSYCHO_NONNULL static bool dma_config_valid(struct psycho_ctx *ctx, enum psycho_dmac_channel_id id)
{
	struct psycho_dmac_channel *ch	  = &ctx->dmac.channels[id];
	const struct channel_route *route = &route_cfg[id];

	const char *str = channel_to_str(id);

	unsigned int syncmode = field_get32(CHCR_SYNCMODE_MASK, ch->chcr);

	if (unlikely(!route->mode_handlers[syncmode])) {
		LOG_ERR(ctx, "\"%s\": invalid channel config: no mode handler assigned for syncmode %u; not running",
			str, syncmode);

		return false;
	}

	bool needs_dreq = ((syncmode == CHCR_SYNCMODE_BURST) || (syncmode == CHCR_SYNCMODE_SLICE)) &&
			  !(ch->chcr & CHCR_FORCE_TRANSFER_START_WITHOUT_DREQ);

	if (unlikely(needs_dreq && !route->dreq_line)) {
		LOG_ERR(ctx,
			"\"%s\": invalid channel config: burst/slice mode with a device-requested (DREQ) transfer "
			"needs a DREQ line, but the device has none; not running",
			str);

		return false;
	}

	if (id == PSYCHO_DMAC_CHANNEL_ID_OTC)
		// Directions are nonsense on the OTC side
		goto ok;

	if (unlikely((ch->chcr & CHCR_TRANSFER_DIR) && !route->dst)) {
		LOG_ERR(ctx,
			"\"%s\": invalid channel config: RAM to device transfer direction requested but no destination "
			"route is set up; not running",
			str);

		return false;
	}

	if (unlikely(!(ch->chcr & CHCR_TRANSFER_DIR) && !route->src)) {
		LOG_ERR(ctx,
			"\"%s\": invalid channel config: device to RAM transfer direction requested but no source "
			"route is set up; not running",
			str);
		return false;
	}

ok:
	LOG_TRACE(ctx, "\"%s\" config ok", str);
	return true;
}

static int dma_ch_priority_sort_cmp(const void *a, const void *b)
{
	unsigned int ch_prio_first  = *(const unsigned int *)a;
	unsigned int ch_prio_second = *(const unsigned int *)b;

	return (ch_prio_first > ch_prio_second) - (ch_prio_first < ch_prio_second);
}

PSYCHO_NONNULL static void transfer_event(struct psycho_ctx *ctx, void *)
{
	struct psycho_dmac_channel *ch = &ctx->dmac.channels[ctx->dmac.active_channel_id];
	uint32_t sync_mode	       = field_get32(CHCR_SYNCMODE_MASK, ch->chcr);

	route_cfg[ctx->dmac.active_channel_id].mode_handlers[sync_mode](ctx);
}

[[nodiscard]] PSYCHO_NONNULL static unsigned int transfer_event_ts(struct psycho_ctx *ctx,
								   enum psycho_dmac_channel_id id, uint32_t chcr)
{
	uint32_t bcr		   = ctx->dmac.channels[id].bcr;
	unsigned int clks_per_word = route_cfg[id].clks_per_word;

	switch (field_get32(CHCR_SYNCMODE_MASK, chcr)) {
	case CHCR_SYNCMODE_BURST:
		return clks_per_word * burst_word_count(bcr);

	case CHCR_SYNCMODE_SLICE:
		return clks_per_word * field_get32(BA_MASK, bcr);

	case CHCR_SYNCMODE_LINKED_LIST:
		return field_get32(BC_BS_MASK, bcr);

	default:
		unreachable();
	}
}

PSYCHO_NONNULL static void dma_transfer_event_start(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel,
						    uint32_t word)
{
	ctx->dmac.active_channel_id = channel;

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

[[nodiscard]] static bool channel_master_enabled(uint32_t dpcr, enum psycho_dmac_channel_id ch)
{
	return dpcr & (1 << (3 + (ch * 4)));
}

[[nodiscard]] static bool channel_master_became_enabled(uint32_t prev_dpcr, uint32_t new_dpcr,
							enum psycho_dmac_channel_id ch)
{
	return (!channel_master_enabled(prev_dpcr, ch)) && (channel_master_enabled(new_dpcr, ch));
}

static bool dma_transfer_init(struct psycho_ctx *ctx, enum psycho_dmac_channel_id id, uint32_t chcr)
{
	struct psycho_dmac_channel *ch = &ctx->dmac.channels[id];
	const char *str		       = channel_to_str(id);

	if (!dma_config_valid(ctx, id))
		return false;

	if (chcr & CHCR_FORCE_TRANSFER_START_WITHOUT_DREQ) {
		LOG_TRACE(ctx, "starting DMA transfer, not waiting for DREQ (channel \"%s\")", str);
		goto start;
	}

	if (route_cfg[id].dreq_line(ctx)) {
		LOG_TRACE(ctx, "starting DMA transfer as DREQ is already asserted (channel \"%s\")", str);
		goto start;
	}

	LOG_TRACE(ctx, "DMA transfer latched (channel \"%s\") - waiting for DREQ", str);
	return true;

start:
	dma_transfer_event_start(ctx, id, chcr);
	return true;
}

void psycho_dmac_reset(struct psycho_ctx *ctx)
{
	ctx->dmac.dpcr = 0x07654321;
	ctx->dmac.channels[PSYCHO_DMAC_CHANNEL_ID_OTC].chcr |= CHCR_OTC_FIXED_BITS;

	update_priorities(ctx);
}

void psycho_dmac_channel_madr_write(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, uint32_t madr)
{
	madr &= MADR_MASK;

	ctx->dmac.channels[channel].madr = madr;
	LOG_TRACE(ctx, "%s MADR set to 0x%08X", channel_to_str(channel), madr);
}

void psycho_dmac_channel_bcr_write(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, uint32_t bcr)
{
	ctx->dmac.channels[channel].bcr = bcr;
	LOG_TRACE(ctx, "%s BCR set to 0x%08X", channel_to_str(channel), bcr);
}

void psycho_dmac_channel_chcr_write(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, uint32_t word)
{
	struct psycho_dmac_channel *ch = &ctx->dmac.channels[channel];
	const char *str		       = channel_to_str(channel);

	if (channel == PSYCHO_DMAC_CHANNEL_ID_OTC)
		word = (word & CHCR_OTC_BITS) | CHCR_OTC_FIXED_BITS;
	else
		word &= CHCR_BITS;

	bool start_requested = bits_became_set(ch->chcr, word, CHCR_START_TRANSFER);

	LOG_TRACE(ctx, "%s CHCR set to 0x%08X", str, word);
	ch->chcr = word;

	if (likely(!start_requested))
		return;

	if (unlikely(!channel_master_enabled(ctx->dmac.dpcr, channel))) {
		LOG_WARN(ctx, "transfer requested on channel \"%s\" but not enabled in DPCR", str);
		return;
	}

	if (ctx->dmac.transfer_event.valid) {
		LOG_WARN(ctx, "dma transfer ongoing - transfer may take effect later");
		return;
	}

	dma_transfer_init(ctx, channel, word);
}

void psycho_dmac_dpcr_write(struct psycho_ctx *ctx, uint32_t word)
{
	// XXX: What happens when a DMA channel's master enable is disabled while the DMA transfer is running?
	uint32_t prev_dpcr = ctx->dmac.dpcr;
	ctx->dmac.dpcr	   = word;

	update_priorities(ctx);

	// Go through the channels that are enabled, and check if a transfer is pending.

	if (ctx->dmac.transfer_event.valid)
		// Transfer is already running, don't bother.
		return;

	for (size_t i = 0; i < ctx->dmac.num_channels_enabled; ++i) {
		enum psycho_dmac_channel_id ch = ctx->dmac.channels_by_priority[i];

		if (!channel_master_became_enabled(prev_dpcr, word, ch))
			continue;

		if (!(ctx->dmac.channels[ch].chcr & CHCR_START_TRANSFER))
			continue;

		// Master enable was not previously not enabled for this channel, but it has been now.
		dma_transfer_init(ctx, ch, ctx->dmac.channels[ch].chcr);
		break;
	}
}

void psycho_dmac_dicr_write(struct psycho_ctx *ctx, uint32_t word)
{
	bool prev = ctx->dmac.dicr & DICR_MASTER_INTERRUPT_FLAG;

	uint32_t flags = ctx->dmac.dicr & DICR_CHANNEL_INTERRUPT_FLAGS & ~word;
	ctx->dmac.dicr = flags | (word & DICR_WRITE_BITS);

	dicr_update(ctx, prev);
}

void psycho_dmac_channel_dreq_update(struct psycho_ctx *ctx, enum psycho_dmac_channel_id channel, bool asserted)
{
	if (!(ctx->dmac.channels[channel].chcr & CHCR_START_TRANSFER))
		return;

	if (asserted) {
		if (ctx->dmac.transfer_event.valid) {
			__builtin_trap();
		}
		dma_transfer_event_start(ctx, channel, ctx->dmac.channels[channel].chcr);
	}
}
