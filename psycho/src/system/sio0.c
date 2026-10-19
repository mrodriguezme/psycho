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

#include <string.h>

#include "common/sched.h"
#include "common/util.h"

#include "debug/log.h"

#include "intc.h"
#include "sio0.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_SIO0);

#define SR_TXRDY  (1 << 0)
#define SR_RXRDY  (1 << 1)
#define SR_TXU	  (1 << 2)
#define SR_PERROR (1 << 3)
#define SR_DSR	  (1 << 7)
#define SR_IRQ	  (1 << 9)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

#define SR_BAUDTMR_MASK	 (0xFFFFF800)
#define SR_BAUDTMR_SHIFT (11)

#pragma GCC diagnostic pop

#define MR_BR_MASK	     (0x3)
#define MR_BR_SHIFT	     (0)
#define MR_BR_MUL1_0	     (0)
#define MR_BR_MUL1_1	     (1)
#define MR_BR_MUL16	     (2)
#define MR_BR_MUL64	     (3)

#define MR_CHLEN_MASK	     (0xC)
#define MR_CHLEN_SHIFT	     (2)
#define MR_CHLEN_5BIT	     (0)
#define MR_CHLEN_6BIT	     (1)
#define MR_CHLEN_7BIT	     (2)
#define MR_CHLEN_8BIT	     (3)

#define MR_PEN		     (1 << 4)
#define MR_P		     (1 << 5)
#define MR_CPOL		     (1 << 8)

#define MR_BITS		     (MR_BR_MASK | MR_CHLEN_MASK | MR_PEN | MR_P | MR_CPOL)

// All controllers and memory cards expect the following settings:
//
// * character length shall be set to 8
// * the clock polarity should be set to high-when-idle
// * parity should be disabled
//
// Any settings other than these WILL NOT WORK on all known official hardware.
#define MR_OFFICIAL	     (MR_CHLEN_8BIT << MR_CHLEN_SHIFT)

#define CR_TXEN		     (1 << 0)
#define CR_DTR		     (1 << 1)
#define CR_RXEN		     (1 << 2)
#define CR_ERRRST	     (1 << 4)
#define CR_INTRST	     (1 << 6)

#define CR_BUFSZ_MASK	     (0x300)
#define CR_BUFSZ_SHIFT	     (8)
#define CR_BUFSZ_ONE_BYTE    (0)
#define CR_BUFSZ_TWO_BYTES   (1)
#define CR_BUFSZ_FOUR_BYTES  (2)
#define CR_BUFSZ_EIGHT_BYTES (3)

#define CR_TXIEN	     (1 << 10)
#define CR_RXIEN	     (1 << 11)
#define CR_DSRIEN	     (1 << 12)
#define CR_PORT		     (1 << 13)

enum {
	HI_Z = UINT8_MAX,
};

PSYCHO_NONNULL static void irq_assert(struct psycho_ctx *ctx)
{
	if (unlikely(ctx->sio0.sr & SR_IRQ))
		return;

	ctx->sio0.sr |= SR_IRQ;
	psycho_intc_irqs_pending_set(ctx, IRQ_SIO0);

	LOG_TRACE(ctx, "IRQ asserted");
}

PSYCHO_NODISCARD PSYCHO_NONNULL static enum psycho_sio0_slot asserted_slot_get(struct psycho_ctx *ctx)
{
	return !(ctx->sio0.cr & CR_PORT) ? PSYCHO_SIO0_SLOT_1 : PSYCHO_SIO0_SLOT_2;
}

PSYCHO_NODISCARD PSYCHO_NONNULL static unsigned int mr_br_get(struct psycho_ctx *ctx)
{
	static const unsigned int mr_br_lut[] = {
		// clang-format off

		[MR_BR_MUL1_0 ... MR_BR_MUL1_1]	= 1,
		[MR_BR_MUL16]			= 16,
		[MR_BR_MUL64]			= 64

		// clang-format on
	};

	return mr_br_lut[(ctx->sio0.mr & MR_BR_MASK) >> MR_BR_SHIFT];
}

PSYCHO_NODISCARD PSYCHO_NONNULL static unsigned int cr_bufsz_get(struct psycho_ctx *ctx)
{
	static const unsigned int cr_bufsz_lut[] = {
		// clang-format off

		[CR_BUFSZ_ONE_BYTE]	= 1,
		[CR_BUFSZ_TWO_BYTES]	= 2,
		[CR_BUFSZ_FOUR_BYTES]	= 4,
		[CR_BUFSZ_EIGHT_BYTES]	= 8

		// clang-format on
	};

	return cr_bufsz_lut[(ctx->sio0.cr & CR_BUFSZ_MASK) >> CR_BUFSZ_SHIFT];
}

PSYCHO_NODISCARD PSYCHO_NONNULL static unsigned int word_len_get(struct psycho_ctx *ctx)
{
	return 5 + ((ctx->sio0.mr & MR_CHLEN_MASK) >> MR_CHLEN_SHIFT);
}

PSYCHO_NONNULL static void devices_reset(struct psycho_ctx *ctx)
{
	enum psycho_sio0_slot slot = asserted_slot_get(ctx);

	for (size_t i = 0; i < PSYCHO_SIO0_DEVICE_TYPE_COUNT; ++i) {
		struct psycho_sio0_device *device = ctx->sio0.devices[slot][i];

		if (!device)
			continue;

		device->reset(device);

		psycho_sched_del(ctx, &device->ack_pulse_assert_event);
		psycho_sched_del(ctx, &device->ack_pulse_deassert_event);

		LOG_DEBUG(ctx, "device \"%s\" reset in slot %u", device->name, slot + 1);
	}
}

PSYCHO_NODISCARD PSYCHO_NONNULL static unsigned int calc_baud_rate(struct psycho_ctx *ctx, uint16_t baud)
{
	baud *= mr_br_get(ctx);
	return PSYCHO_CPU_CLKFREQ_HZ / baud;
}

PSYCHO_NONNULL static void rx_push(struct psycho_ctx *ctx, uint8_t byte)
{
	LOG_TRACE(ctx, "RX FIFO push request: 0x%02X", byte);

	size_t num_entries = ctx->sio0.rxfifo.num_entries;

	if (unlikely(ctx->sio0.rxfifo.num_entries))
		LOG_WARN(ctx, "RX FIFO contains %zu %s", num_entries, (num_entries > 1) ? "entries" : "entry");

	size_t cnt = ARRAY_SIZE(ctx->sio0.rxfifo.entries);

	if (unlikely(num_entries >= cnt)) {
		// When receiving bytes while the RX FIFO is full, then the last FIFO entry will by overwritten by the
		// new byte.
		cnt--;

		uint8_t old = ctx->sio0.rxfifo.entries[cnt];

		ctx->sio0.rxfifo.entries[cnt] = byte;

		LOG_WARN(ctx, "RX FIFO overflow; replaced last RX FIFO entry 0x%02X with 0x%02X", old, byte);
		return;
	}

	ctx->sio0.rxfifo.entries[ctx->sio0.rxfifo.num_entries++] = byte;
	ctx->sio0.sr |= SR_RXRDY;

	if (ctx->sio0.cr & CR_RXIEN)
		if (ctx->sio0.rxfifo.num_entries == cr_bufsz_get(ctx))
			irq_assert(ctx);

	LOG_TRACE(ctx, "Pushed 0x%02X to RX FIFO", byte);
}

static void transceive_event(struct psycho_ctx *ctx, void *userdata)
{
	(void)userdata;

	enum psycho_sio0_slot slot = asserted_slot_get(ctx);

	uint8_t miso = HI_Z;

	for (size_t i = 0; i < PSYCHO_SIO0_DEVICE_TYPE_COUNT; ++i) {
		struct psycho_sio0_device *device = ctx->sio0.devices[slot][i];

		if (!device)
			continue;

		// All devices on the bus will see the transmitted byte even if they haven't been addressed, but only
		// the addressed device's response will end up in the RXFIFO. At the beginning of every transaction (CS
		// going low), HI-Z is guaranteed (0xFF).
		uint8_t ret = device->transceive(device->handle, ctx->sio0.txfifo.latched);

		if (ctx->sio0.addressed_device == device)
			miso = ret;
	}

	rx_push(ctx, miso);
	ctx->sio0.sr |= SR_TXU;
}

PSYCHO_NONNULL static void do_tx(struct psycho_ctx *ctx)
{
	unsigned int mr_br    = mr_br_get(ctx);
	unsigned int word_len = word_len_get(ctx);

	ctx->sio0.tx_event.ts = ctx->sio0.br * mr_br * word_len;

	ctx->sio0.tx_event.cb	     = transceive_event;
	ctx->sio0.tx_event.type	     = PSYCHO_SCHED_EVENT_TYPE_SIO0_TX;
	ctx->sio0.tx_event.permanent = false;

	psycho_sched_add(ctx, &ctx->sio0.tx_event);
	ctx->sio0.sr &= ~SR_TXU;
}

void psycho_sio0_dsr_assert(struct psycho_ctx *ctx, void *dev)
{
	enum psycho_sio0_slot slot = asserted_slot_get(ctx) + 1;

	if (unlikely(!ctx->sio0.addressed_device)) {
		ctx->sio0.addressed_device = dev;
		LOG_TRACE(ctx, "device \"%s\" in slot %u addressed", ctx->sio0.addressed_device->name, slot);
	}

	LOG_TRACE(ctx, "device \"%s\" in slot %u asserted ACK; DSR asserted", ctx->sio0.addressed_device->name, slot);

	ctx->sio0.sr |= SR_DSR;

	if (likely(ctx->sio0.cr & CR_DSRIEN))
		irq_assert(ctx);
}

void psycho_sio0_dsr_deassert(struct psycho_ctx *ctx, void *userdata)
{
	(void)userdata;

	enum psycho_sio0_slot slot = asserted_slot_get(ctx) + 1;

	LOG_TRACE(ctx, "device \"%s\" in slot %u deasserted ACK; DSR deasserted", ctx->sio0.addressed_device->name,
		  slot);

	ctx->sio0.sr &= ~SR_DSR;
}

void psycho_sio0_reset(struct psycho_ctx *ctx)
{
	psycho_sched_del(ctx, &ctx->sio0.tx_event);

	memset(&ctx->sio0.rxfifo, 0, sizeof(ctx->sio0.rxfifo));
	memset(&ctx->sio0.txfifo, 0, sizeof(ctx->sio0.txfifo));

	ctx->sio0.sr |= (SR_TXRDY | SR_TXU);
	ctx->sio0.sr &= ~SR_IRQ;

	psycho_sio0_mr_write_halfword(ctx, 0x000D);
	psycho_sio0_br_write_halfword(ctx, 0x0088);
}

void psycho_sio0_dr_write_byte(struct psycho_ctx *ctx, uint8_t byte)
{
	LOG_TRACE(ctx, "TX FIFO push request: 0x%02X", byte);

	if (unlikely(!(ctx->sio0.cr & CR_TXEN))) {
		LOG_WARN(ctx, "TXEN disabled; dropping request (actual behavior is not known yet)");
		return;
	}

	if (unlikely(!(ctx->sio0.sr & SR_TXRDY))) {
		// Writing to this register while SIO_STAT.0=Busy causes the old value to be overwritten.
		LOG_WARN(ctx, "TX FIFO overflow; replacing old entry 0x%02X with 0x%02X", ctx->sio0.txfifo.entry, byte);

		ctx->sio0.txfifo.entry = byte;
		return;
	}

	ctx->sio0.txfifo.latched = byte;
	do_tx(ctx);
}

uint8_t psycho_sio0_dr_read_byte(struct psycho_ctx *ctx)
{
	if (unlikely(!ctx->sio0.rxfifo.num_entries)) {
		// Reading from Empty RX FIFO returns either the most recently received byte or zero.
		LOG_WARN(ctx, "RX FIFO underflow; returning last RX byte 0x%02X", ctx->sio0.last_rx);
		return ctx->sio0.last_rx;
	}

	uint8_t byte = ctx->sio0.rxfifo.entries[0];
	LOG_TRACE(ctx, "RX FIFO popped; returning 0x%02X", byte);

	ctx->sio0.rxfifo.num_entries--;

	for (size_t i = 0; i < ctx->sio0.rxfifo.num_entries; ++i)
		ctx->sio0.rxfifo.entries[i] = ctx->sio0.rxfifo.entries[i + 1];

	if (unlikely(!ctx->sio0.rxfifo.num_entries))
		ctx->sio0.sr &= ~SR_RXRDY;

	ctx->sio0.last_rx = byte;
	return byte;
}

void psycho_sio0_mr_write_halfword(struct psycho_ctx *ctx, uint16_t mr)
{
	mr &= MR_BITS;

	static const char *mr_br_str[] = {
		// clang-format off

		[MR_BR_MUL1_0 ... MR_BR_MUL1_1]	= "MUL1",
		[MR_BR_MUL16]			= "MUL16",
		[MR_BR_MUL64]			= "MUL64"

		// clang-format on
	};

	static const char *mr_chlen_str[] = {
		// clang-format off

		[MR_CHLEN_5BIT]	= "5 bits",
		[MR_CHLEN_6BIT]	= "6 bits",
		[MR_CHLEN_7BIT]	= "7 bits",
		[MR_CHLEN_8BIT]	= "8 bits"

		// clang-format on
	};

	const char *br	  = mr_br_str[(mr & MR_BR_MASK) >> MR_BR_SHIFT];
	const char *chlen = mr_chlen_str[(mr & MR_CHLEN_MASK) >> MR_CHLEN_SHIFT];
	const char *pen	  = (mr & MR_PEN) ? "enabled" : "disabled";
	const char *p	  = (mr & MR_P) ? "even" : "odd";
	const char *cpol  = (mr & MR_CPOL) ? "low when idle" : "high when idle";

	LOG_DEBUG(ctx, "mr set to 0x%04X (br = %s, chlen = %s, pen = %s, p = %s, cpol = %s)", mr, br, chlen, pen, p,
		  cpol);

	if (unlikely((mr & (MR_CHLEN_MASK | MR_CPOL | MR_PEN)) != MR_OFFICIAL))
		LOG_WARN(ctx, "mr has been set to settings that are incompatible with all known official devices; this "
			      "would not work on a real playstation");

	ctx->sio0.mr = mr;
}

void psycho_sio0_cr_write_halfword(struct psycho_ctx *ctx, uint16_t cr)
{
	static const char *cr_bufsz_lut[] = {
		// clang-format off

		[CR_BUFSZ_ONE_BYTE]	= "1 byte",
		[CR_BUFSZ_TWO_BYTES]	= "2 bytes",
		[CR_BUFSZ_FOUR_BYTES]	= "4 bytes",
		[CR_BUFSZ_EIGHT_BYTES]	= "8 bytes"

		// clang-format on
	};

	if (cr & CR_ERRRST) {
		ctx->sio0.sr &= ~(SR_PERROR | SR_IRQ);
		LOG_DEBUG(ctx, "IRQ acknowledged; SR_PERROR and SR_IRQ reset");
	}

	if (cr & CR_INTRST) {
		psycho_sio0_reset(ctx);
		LOG_DEBUG(ctx, "reset by CR_INTRST bit; behavior questionable");

		cr &= ~CR_INTRST;
	}

	const char *txen = (cr & CR_TXEN) ? "enabled" : "disabled";
	const char *dtr	 = (cr & CR_DTR) ? "enabled" : "disabled";
	const char *rxen = (cr & CR_RXEN) ? "enabled (force receiving single byte)" :
					    "disabled (only receive when /CS low)";

	const char *bufsz  = cr_bufsz_lut[(cr & CR_BUFSZ_MASK) >> CR_BUFSZ_SHIFT];
	const char *txien  = (cr & CR_TXIEN) ? "enabled" : "disabled";
	const char *rxien  = (cr & CR_RXIEN) ? "enabled" : "disabled";
	const char *dsrien = (cr & CR_DSRIEN) ? "enabled" : "disabled";
	const char *port   = (cr & CR_PORT) ? "port 2" : "port 1";

	LOG_DEBUG(ctx,
		  "cr set to 0x%04X (txen = %s, dtr = %s, rxen = %s, bufsz = %s, txien = %s, rxien = %s, dsrien = %s, "
		  "port = %s)",
		  cr, txen, dtr, rxen, bufsz, txien, rxien, dsrien, port);

	if (bits_became_set(ctx->sio0.cr, cr, CR_DTR)) {
		LOG_DEBUG(ctx, "DTR output level set high; CS has gone low");
		devices_reset(ctx);
	} else if (bits_became_clr(ctx->sio0.cr, cr, CR_DTR)) {
		LOG_DEBUG(ctx, "DTR output level set low; CS has gone high");
		ctx->sio0.addressed_device = NULL;
	}

	ctx->sio0.cr = cr;
}

void psycho_sio0_br_write_halfword(struct psycho_ctx *ctx, uint16_t br)
{
	unsigned int bps = calc_baud_rate(ctx, br);

	LOG_INFO(ctx, "baud reload value set to %d (new baud rate = %u bps)", br, bps);

	ctx->sio0.br = br;
}
