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

#include "psycho/compiler.h"
#include "sio0.h"
#include "log.h"
#include "util.h"
#include "sched.h"

LOG_MOD(P_LOG_SIO0);

#define SIO0_STAT_TX_FIFO_NOT_FULL (1 << 0)
#define SIO0_STAT_RX_FIFO_NOT_EMPTY (1 << 1)
#define SIO0_STAT_TX_IDLE (1 << 2)
#define SIO0_STAT_RX_PAR_ERR (1 << 3)
#define SIO0_STAT_DSR_IN_LVL (1 << 7)
#define SIO0_STAT_IRQ (1 << 9)
#define SIO0_BAUD_RATE_TMR_MASK (0b11111111111111111111100000000000)
#define SIO0_BAUD_RATE_TMR_SHIFT (11)

enum {
	// clang-format off

	SIO0_MODE_BAUD_RELOAD_MASK	= 0b0000000000000011,
	SIO0_MODE_BAUD_RELOAD_SHIFT	= 0,
	SIO0_MODE_CHAR_LEN_MASK		= 0b0000000000001100,
	SIO0_MODE_CHAR_LEN_SHIFT	= 2,
	SIO0_MODE_PAR_EN		= 1 << 4,
	SIO0_MODE_PAR_TYPE		= 1 << 5,
	SIO0_MODE_CPOL			= 1 << 8

	// clang-format on
};

enum {
	// clang-format off

	SIO0_CTRL_TXEN			= 1 << 0,
	SIO0_CTRL_DTR_OUT_LVL		= 1 << 1,
	SIO0_CTRL_RXEN			= 1 << 2,
	SIO0_CTRL_ACK			= 1 << 4,
	SIO0_CTRL_RX_INT_MODE_MASK	= 0b0000001100000000,
	SIO0_CTRL_RX_INT_MODE_SHIFT	= 8,
	SIO0_CTRL_TX_INT_EN		= 1 << 10,
	SIO0_CTRL_RX_INT_EN		= 1 << 11,
	SIO0_CTRL_DSR_INT_EN		= 1 << 12,
	SIO0_CTRL_PORT_SEL		= 1 << 13

	// clang-format on
};

enum {
	SIO0_DEV_TGT_SETTINGS = (00000000000001100) | !SIO0_MODE_PAR_EN |
				!SIO0_MODE_CPOL,
};

static void transceive_event(struct p_ctx *ctx);

void p_sio0_rst(struct p_ctx *ctx)
{
	ctx->sio0.baud = 0x00DC;
	ctx->sio0.mode = 0x004E;
	ctx->sio0.stat = SIO0_STAT_TX_FIFO_NOT_FULL | SIO0_STAT_TX_IDLE;
}

P_NODISCARD __attribute__((nonnull)) static enum sio0_slot
selected_slot(const struct p_ctx *ctx)
{
	return !(ctx->sio0.ctrl & SIO0_CTRL_PORT_SEL) ? SLOT_1 : SLOT_2;
}

static void rx_push(struct p_ctx *ctx, const u8 byte)
{
	if (unlikely(ctx->sio0.rxfifo.num_entries))
		LOG_WARN(ctx, "rx fifo has %zu %s - this is highly unusual",
			 ctx->sio0.rxfifo.num_entries,
			 (ctx->sio0.rxfifo.num_entries > 1) ? "entries" :
							      "entry");

	if (unlikely((ctx->sio0.rxfifo.num_entries) >=
		     ARRAY_SIZE(ctx->sio0.rxfifo.entries))) {
		LOG_WARN(ctx, "rx fifo overflow");
		ctx->sio0.rxfifo.num_entries = 0;
	}

	ctx->sio0.rxfifo.entries[ctx->sio0.rxfifo.num_entries++] = byte;
	ctx->sio0.stat |= SIO0_STAT_RX_FIFO_NOT_EMPTY;

	LOG_WARN(ctx, "rxfifo <- 0x%02X", byte);
}

P_NONNULL static void tx(struct p_ctx *ctx)
{
	ctx->sio0.tx_ev.ts = 1018;
	ctx->sio0.tx_ev.cb = transceive_event;
	ctx->sio0.tx_ev.type = P_SCHED_EV_SIO0_TX;
	ctx->sio0.tx_ev.permanent = false;

	p_sched_add(ctx, &ctx->sio0.tx_ev);
	ctx->sio0.stat &= ~SIO0_STAT_TX_FIFO_NOT_FULL;
}

static void transceive_event(struct p_ctx *const ctx)
{
	struct p_sio0_dev *dev;

	if (likely(ctx->sio0.curr_dev)) {
		dev = ctx->sio0.curr_dev;

		bool done;
		const u8 byte =
			dev->transceive(dev->handle, ctx->sio0.curr_tx, &done);

		rx_push(ctx, byte);

		if (done)
			ctx->sio0.curr_dev = NULL;

		goto end;
	}

	ctx->sio0.curr_dev = ctx->sio0.dev[0][0];
	rx_push(ctx, 0xFF);

end:
	if (ctx->sio0.need_tx) {
		ctx->sio0.curr_tx = ctx->sio0.tx;
		tx(ctx);
	} else
		ctx->sio0.stat |= SIO0_STAT_TX_FIFO_NOT_FULL;
}

void p_sio0_tx(struct p_ctx *ctx, u8 byte)
{
	LOG_WARN(ctx, "txfifo <- 0x%02X", byte);

	if (!(ctx->sio0.stat & SIO0_STAT_TX_FIFO_NOT_FULL)) {
		// Writing to this register while SIO_STAT.0=Busy causes the old
		// value to be overwritten.
		LOG_WARN(ctx,
			 "overwriting txfifo with 0x%02X as it is full (txfifo "
			 "contained 0x%02X)",
			 byte, ctx->sio0.tx);

		ctx->sio0.tx = byte;
		ctx->sio0.need_tx = true;

		return;
	}

	ctx->sio0.curr_tx = byte;
	tx(ctx);
}

u8 p_sio0_rx_pop8(struct p_ctx *ctx)
{
	if (unlikely(!ctx->sio0.rxfifo.num_entries)) {
		LOG_WARN(ctx, "rx fifo underflow, rxfifo -> 0x00");
		return 0xFF;
	}

	u8 byte = ctx->sio0.rxfifo.entries[0];

	ctx->sio0.rxfifo.num_entries--;

	for (size_t i = 0; i < ctx->sio0.rxfifo.num_entries; i++)
		ctx->sio0.rxfifo.entries[i] = ctx->sio0.rxfifo.entries[i + 1];

	if (!ctx->sio0.rxfifo.num_entries)
		ctx->sio0.stat &= ~SIO0_STAT_RX_FIFO_NOT_EMPTY;

	LOG_WARN(ctx, "rxfifo -> 0x%02X", byte);
	return byte;
}

void p_sio0_mode_set(struct p_ctx *ctx, u16 mode)
{
	// Bits 6-7 on SIO0 are always zero
	mode &= ~((1 << 6) | (1 << 7));

	if (!(mode & SIO0_DEV_TGT_SETTINGS)) {
		LOG_WARN(
			ctx,
			"will refuse to transmit to peripherals - target settings are not correct");
		return;
	}

	if (bit_became_set(ctx->sio0.ctrl, mode, SIO0_CTRL_TXEN))
		if (ctx->sio0.need_tx) {
			// There might be a byte in the TXFIFO but the enable
			// bit wasn't set - transmit now
			tx(ctx);
		}

	ctx->sio0.mode = mode;
}

void p_sio0_ctrl_set(struct p_ctx *const ctx, const u16 ctrl)
{
	ctx->sio0.ctrl = ctrl;
}

void p_attach_dev_to_sio0(struct p_ctx *const ctx, struct p_sio0_dev *const dev,
			  const enum sio0_slot slot)
{
	if (ctx->sio0.dev[slot][dev->type]) {
		LOG_WARN(ctx,
			 "attempting to attach device of same type to the same "
			 "port");
		return;
	}

	ctx->sio0.dev[slot][dev->type] = dev;

	LOG_INFO(ctx, "attached peripheral \"%s\" to slot %u (type = %s)",
		 dev->name, slot,
		 dev->type == P_SIO0_DEV_TYPE_CTRL ? "controller" : "memcard");
}
