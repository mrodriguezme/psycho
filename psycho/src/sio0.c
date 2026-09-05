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

#include <string.h>

#include "sio0.h"
#include "log.h"
#include "util.h"
#include "sched.h"
#include "intctrl.h"

LOG_MOD(P_LOG_SIO0);

#define STAT_TX_FIFO_NOT_FULL	      (1 << 0)
#define STAT_RX_FIFO_NOT_EMPTY	      (1 << 1)
#define STAT_TX_IDLE		      (1 << 2)
#define STAT_RX_PAR_ERR		      (1 << 3)
#define STAT_DSR_IN_LVL		      (1 << 7)
#define STAT_IRQ		      (1 << 9)
#define STAT_BAUD_TMR_MASK	      (0b11111111111111111111100000000000)
#define STAT_BAUD_TMR_SHIFT	      (11)

#define MODE_BAUD_RELOAD_FACTOR_MASK  (0b0000000000000011)
#define MODE_BAUD_RELOAD_FACTOR_SHIFT (0)
#define MODE_BAUD_RELOAD_MUL1_0	      (0)
#define MODE_BAUD_RELOAD_MUL1_1	      (1)
#define MODE_BAUD_RELOAD_MUL16	      (2)
#define MODE_BAUD_RELOAD_MUL64	      (3)

#define MODE_CHAR_LEN_MASK	      (0b0000000000001100)
#define MODE_CHAR_LEN_SHIFT	      (2)
#define MODE_CHAR_LEN_5BIT	      (0)
#define MODE_CHAR_LEN_6BIT	      (1)
#define MODE_CHAR_LEN_7BIT	      (2)
#define MODE_CHAR_LEN_8BIT	      (3)

#define MODE_PAR_EN		      (1 << 4)
#define MODE_PAR_TYPE		      (1 << 5)
#define MODE_CPOL		      (1 << 8)

#define MODE_BITS                                                          \
	(MODE_BAUD_RELOAD_FACTOR_MASK | MODE_CHAR_LEN_MASK | MODE_PAR_EN | \
	 MODE_PAR_TYPE | MODE_CPOL)

// All controllers and memory cards expect the following settings:
//
// * character length shall be set to 8
// * the clock polarity should be set to high-when-idle
// * parity should be disabled
//
// Any settings other than these WILL NOT WORK on all known official hardware.
#define MODE_EXPECTED		    (MODE_CHAR_LEN_8BIT << MODE_CHAR_LEN_SHIFT)

#define CTRL_TXEN		    (1 << 0)
#define CTRL_DTR_OUT_LVL	    (1 << 1)
#define CTRL_RXEN		    (1 << 2)
#define CTRL_ACK		    (1 << 4)
#define CTRL_RESET		    (1 << 6)

#define CTRL_RX_INT_MODE_MASK	    (0b0000001100000000)
#define CTRL_RX_INT_MODE_SHIFT	    (8)
#define CTRL_RX_INT_IRQ_ONE_BYTE    (0)
#define CTRL_RX_INT_IRQ_TWO_BYTES   (1)
#define CTRL_RX_INT_IRQ_FOUR_BYTES  (2)
#define CTRL_RX_INT_IRQ_EIGHT_BYTES (3)

#define CTRL_TX_INT_EN		    (1 << 10)
#define CTRL_RX_INT_EN		    (1 << 11)
#define CTRL_DSR_INT_EN		    (1 << 12)
#define CTRL_PORT_SEL		    (1 << 13)

static void transceive_event(struct p_ctx *ctx);
static void ack_event(struct p_ctx *ctx);

/**
 * @brief Raise the SIO0 interrupt.
 *
 * Sets STAT_IRQ in the status register and asserts the SIO0 line on the
 * interrupt controller.
 *
 * @param ctx Emulator context.
 */
P_NONNULL static void raise_irq(struct p_ctx *ctx)
{
	ctx->sio0.stat |= STAT_IRQ;
	p_irq_pend(ctx, IRQ_SIO0);
}

/**
 * @brief Determine which peripheral slot is currently selected.
 *
 * Reflects the state of the CTRL_PORT_SEL bit in SIO0_CTRL: peripherals
 * connected to the non-selected slot do not receive the clock/data lines and
 * will not respond to transactions.
 *
 * @param ctx Emulator context.
 * @return The currently selected slot (SLOT_1 or SLOT_2).
 */
P_NODISCARD P_NONNULL static enum sio0_slot selected_slot(struct p_ctx *ctx)
{
	return !(ctx->sio0.ctrl & CTRL_PORT_SEL) ? SLOT_1 : SLOT_2;
}

/**
 * @brief Reset all peripherals attached to the currently selected slot.
 *
 * Iterates every device index for the slot chosen by selected_slot(), invoking
 * each attached device's reset() callback and clearing its "active" (addressed)
 * state. Devices in the non-selected slot are left untouched.
 *
 * @param ctx Emulator context.
 */
P_NONNULL static void reset_peripherals(struct p_ctx *ctx)
{
	enum sio0_slot slot = selected_slot(ctx);

	for (size_t i = 0; i < NUM_DEVS; ++i) {
		struct p_sio0_dev *dev = ctx->sio0.dev[slot][i];

		if (!dev)
			continue;

		dev->reset(dev);
		dev->active = false;

		LOG_DBG(ctx, "peripheral %zu (\"%s\") reset in slot %u", i + 1,
			dev->name, slot);
	}
}

/**
 * @brief Get the baud rate reload multiplier from the current mode register.
 *
 * @param ctx Emulator context.
 * @return The multiplier (1, 16, or 64) selected by the mode register's baud
 *         reload factor field.
 */
P_NODISCARD P_NONNULL static uint baud_fact_get(struct p_ctx *ctx)
{
	static const uint mul[] = {
		[MODE_BAUD_RELOAD_MUL1_0... MODE_BAUD_RELOAD_MUL1_1] = 1,
		[MODE_BAUD_RELOAD_MUL16]			     = 16,
		[MODE_BAUD_RELOAD_MUL64]			     = 64
	};

	return mul[(ctx->sio0.mode & MODE_BAUD_RELOAD_FACTOR_MASK) >>
		   MODE_BAUD_RELOAD_FACTOR_SHIFT];
}

/**
 * @brief Get the number of bits per transmitted/received word.
 *
 * Decodes the character length field from the MODE register: the raw field
 * value (0-3, MODE_CHAR_LEN_5BIT..MODE_CHAR_LEN_8BIT) is added to a base of
 * 5 bits, yielding 5, 6, 7, or 8 bits per word.
 *
 * @param ctx Emulator context.
 * @return Word length in bits (5-8).
 */
P_NODISCARD P_NONNULL static uint word_len_get(struct p_ctx *ctx)
{
	return 5 +
	       ((ctx->sio0.mode & MODE_CHAR_LEN_MASK) >> MODE_CHAR_LEN_SHIFT);
}

/**
 * @brief Get the RX FIFO fill level that should trigger an interrupt.
 *
 * Decodes the CTRL_RX_INT_MODE field from the CTRL register into the
 * corresponding byte count (1, 2, 4, or 8).
 *
 * @param ctx Emulator context.
 * @return Number of bytes the RX FIFO must hold before an RX interrupt is
 *         raised.
 */
P_NODISCARD P_NONNULL static uint rxfifo_intr_lvl_get(struct p_ctx *ctx)
{
	static const uint tbl[] = { [CTRL_RX_INT_IRQ_ONE_BYTE]	  = 1,
				    [CTRL_RX_INT_IRQ_TWO_BYTES]	  = 2,
				    [CTRL_RX_INT_IRQ_FOUR_BYTES]  = 4,
				    [CTRL_RX_INT_IRQ_EIGHT_BYTES] = 8 };

	return tbl[(ctx->sio0.ctrl & CTRL_RX_INT_MODE_MASK) >>
		   CTRL_RX_INT_MODE_SHIFT];
}

/**
 * @brief Convert a baud reload value into an actual bit rate in bps.
 *
 * @param ctx Emulator context.
 * @param baud Raw baud reload value (as written to SIO0_BAUD).
 * @return The resulting bit rate in bits per second.
 */
P_NODISCARD P_NONNULL static uint calc_baud(struct p_ctx *ctx, u16 baud)
{
	baud *= baud_fact_get(ctx);
	return P_CPU_CLKFREQ_HZ / baud;
}

/**
 * @brief Push a received byte into the RX FIFO.
 *
 * Warns if the FIFO already has entries pending (may indicate software isn't
 * draining it fast enough). On overflow, the newest byte overwrites the last
 * slot in the FIFO (rather than being dropped), matching real hardware
 * behavior, and a warning is logged. On a successful push, raises the SIO0
 * interrupt if RX interrupts are enabled and the FIFO has just reached the
 * configured interrupt trigger level.
 *
 * @param ctx  Emulator context.
 * @param byte Byte received from the currently addressed device (or 0xFF if no
 *             device responded).
 */
P_NONNULL static void rx_push(struct p_ctx *ctx, u8 byte)
{
	LOG_TRACE(ctx, "rxfifo push request: 0x%02X", byte);

	size_t num_entries = ctx->sio0.rxfifo.num_entries;

	if (unlikely(ctx->sio0.rxfifo.num_entries)) {
		const char *plural = (num_entries > 1) ? "entries" : "entry";

		LOG_WARN(ctx,
			 "rx fifo has %zu %s - something is probably going "
			 "wrong",
			 num_entries, plural);
	}

	size_t cnt = ARRAY_SIZE(ctx->sio0.rxfifo.entries);

	if (unlikely(num_entries >= cnt)) {
		// When receiving bytes while the RX FIFO is full, then the last
		// FIFO entry will by overwritten by the new byte.
		cnt--;

		u8 old			      = ctx->sio0.rxfifo.entries[cnt];
		ctx->sio0.rxfifo.entries[cnt] = byte;

		LOG_WARN(ctx,
			 "rx fifo overflow; replaced 0x%02X from entries[%zu] "
			 "with 0x%02X; something is probably going wrong",
			 old, cnt, byte);

		return;
	}

	ctx->sio0.rxfifo.entries[ctx->sio0.rxfifo.num_entries++] = byte;
	ctx->sio0.stat |= STAT_RX_FIFO_NOT_EMPTY;

	if (ctx->sio0.ctrl & CTRL_RX_INT_EN)
		if (ctx->sio0.rxfifo.num_entries == rxfifo_intr_lvl_get(ctx))
			raise_irq(ctx);

	LOG_TRACE(ctx, "rxfifo <- 0x%02X", byte);
}

/**
 * @brief Begin transmitting the currently latched TX byte.
 *
 * Schedules transceive_event() to fire once the current byte has finished
 * shifting out, and clears STAT_TX_IDLE while the transfer is in flight.
 * Cancels any previously pending tx_ev first, since p_sched_add() does
 * not permit re-adding an already-valid event.
 *
 * @param ctx Emulator context.
 */
P_NONNULL static void tx(struct p_ctx *ctx)
{
	if (unlikely(ctx->sio0.tx_ev.valid))
		p_sched_del(ctx, &ctx->sio0.tx_ev);

	ctx->sio0.tx_ev.ts =
		ctx->sio0.baud * baud_fact_get(ctx) * word_len_get(ctx);

	ctx->sio0.tx_ev.cb	  = transceive_event;
	ctx->sio0.tx_ev.type	  = P_SCHED_EV_SIO0_TX;
	ctx->sio0.tx_ev.permanent = false;

	p_sched_add(ctx, &ctx->sio0.tx_ev);
	ctx->sio0.stat &= ~STAT_TX_IDLE;
}

void p_sio0_rst(struct p_ctx *ctx)
{
	p_sched_del(ctx, &ctx->sio0.tx_ev);
	p_sched_del(ctx, &ctx->sio0.ack_ev);

	memset(&ctx->sio0.rxfifo, 0, sizeof(ctx->sio0.rxfifo));
	memset(&ctx->sio0.txfifo, 0, sizeof(ctx->sio0.txfifo));

	ctx->sio0.stat |= (STAT_TX_FIFO_NOT_FULL | STAT_TX_IDLE);
	ctx->sio0.stat &= ~STAT_IRQ;

	p_sio0_mode_set(ctx, 0x000D);
	p_sio0_baud_set(ctx, 0x0088);
}

static void ack_event(struct p_ctx *ctx)
{
	LOG_INFO(ctx, "ack!");
}

static void transceive_event(struct p_ctx *ctx)
{
	enum sio0_slot slot = selected_slot(ctx);

	u8 miso	 = 0xFF;
	bool ack = false;

	for (size_t i = 0; i < NUM_DEVS; ++i) {
		struct p_sio0_dev *dev = ctx->sio0.dev[slot][i];

		if (!dev)
			continue;

		// All devices on the bus will see the transmitted byte even if
		// they haven't been addressed.
		u8 ret = dev->transceive(dev->handle, ctx->sio0.txfifo.latched);

		if (dev->active) {
			miso = ret;
			ack  = true;

			continue;
		}

		// This device is currently not being addressed; should it be?
		if (dev->addressed(ctx->sio0.txfifo.latched)) {
			dev->active = true;

			ctx->sio0.curr_dev = dev;
			ack		   = true;
		}
	}

	rx_push(ctx, miso);
	ctx->sio0.stat |= STAT_TX_IDLE;

	if (!ack)
		return;

	if (ctx->sio0.ack_ev.valid)
		p_sched_del(ctx, &ctx->sio0.ack_ev);

	ctx->sio0.ack_ev.ts	   = us_to_cycles(50);
	ctx->sio0.ack_ev.cb	   = ack_event;
	ctx->sio0.ack_ev.type	   = P_SCHED_EV_SIO0_DEV_ACK;
	ctx->sio0.ack_ev.permanent = false;

	p_sched_add(ctx, &ctx->sio0.ack_ev);
}

void p_sio0_tx(struct p_ctx *ctx, u8 byte)
{
	LOG_TRACE(ctx, "tx request <- 0x%02X", byte);

	if (unlikely(!(ctx->sio0.ctrl & CTRL_TXEN))) {
		LOG_WARN(ctx, "TXEN disabled; dropping request (actual "
			      "behavior is not known yet)");
		return;
	}

	if (unlikely(!(ctx->sio0.stat & STAT_TX_FIFO_NOT_FULL))) {
		// Writing to this register while SIO_STAT.0=Busy causes the old
		// value to be overwritten.
		LOG_WARN(ctx,
			 "overwriting txfifo with 0x%02X as it is full (txfifo "
			 "contained 0x%02X)",
			 byte, ctx->sio0.txfifo.entry);

		ctx->sio0.txfifo.entry = byte;
		return;
	}

	ctx->sio0.txfifo.latched = byte;
	tx(ctx);
}

u8 p_sio0_rx_pop8(struct p_ctx *ctx)
{
	if (unlikely(!ctx->sio0.rxfifo.num_entries)) {
		LOG_WARN(ctx, "rx fifo underflow, rxfifo -> 0xFF");
		return 0xFF;
	}

	u8 byte = ctx->sio0.rxfifo.entries[0];

	LOG_TRACE(ctx, "rxfifo -> 0x%02X", byte);

	ctx->sio0.rxfifo.num_entries--;

	for (size_t i = 0; i < ctx->sio0.rxfifo.num_entries; ++i)
		ctx->sio0.rxfifo.entries[i] = ctx->sio0.rxfifo.entries[i + 1];

	if (!ctx->sio0.rxfifo.num_entries)
		ctx->sio0.stat &= ~STAT_RX_FIFO_NOT_EMPTY;

	return byte;
}

void p_sio0_mode_set(struct p_ctx *ctx, u16 mode)
{
	mode &= MODE_BITS;

	static const char *mul_to_str[] = {
		[MODE_BAUD_RELOAD_MUL1_0... MODE_BAUD_RELOAD_MUL1_1] = "MUL1",
		[MODE_BAUD_RELOAD_MUL16]			     = "MUL16",
		[MODE_BAUD_RELOAD_MUL64]			     = "MUL64"
	};

	static const char *char_len_to_str[] = { [MODE_CHAR_LEN_5BIT] = "5",
						 [MODE_CHAR_LEN_6BIT] = "6",
						 [MODE_CHAR_LEN_7BIT] = "7",
						 [MODE_CHAR_LEN_8BIT] = "8" };

	const char *mul = mul_to_str[(mode & MODE_BAUD_RELOAD_FACTOR_MASK) >>
				     MODE_BAUD_RELOAD_FACTOR_SHIFT];

	const char *char_len = char_len_to_str[(mode & MODE_CHAR_LEN_MASK) >>
					       MODE_CHAR_LEN_SHIFT];

	const char *par	     = (mode & MODE_PAR_EN) ? "enabled" : "disabled";
	const char *par_type = (mode & MODE_PAR_TYPE) ? "odd" : "even";
	const char *cpol     = (mode & MODE_CPOL) ? "low when idle" :
						    "high when idle";

	LOG_INFO(ctx,
		 "mode set to 0x%04X (baudrate reload factor = %s, charlen = "
		 "%s bits, parity = %s, parity type = %s, clock polarity = %s)",
		 mode, mul, char_len, par, par_type, cpol);

	if (unlikely((mode & (MODE_CHAR_LEN_MASK | MODE_CPOL | MODE_PAR_EN)) !=
		     MODE_EXPECTED))
		LOG_WARN(ctx,
			 "mode has been set to settings that are incompatible "
			 "with all known official peripherals; this would not "
			 "work on a real system");

	ctx->sio0.mode = mode;
}

void p_sio0_ctrl_set(struct p_ctx *ctx, u16 ctrl)
{
	if (ctrl & CTRL_ACK) {
		ctx->sio0.stat &= ~(STAT_RX_PAR_ERR | STAT_IRQ);
		LOG_DBG(ctx,
			"irq acknowledged; STAT_RX_PAR_ERR and STAT_IRQ reset");
	}

	if (ctrl & CTRL_RESET) {
		p_sio0_rst(ctx);
		LOG_DBG(ctx, "reset by CTRL_RESET bit; behavior questionable");

		ctrl &= ~CTRL_RESET;
	}

	const char *txen = (ctrl & CTRL_TXEN) ? "enabled" : "disabled";
	const char *dtr	 = (ctrl & CTRL_DTR_OUT_LVL) ? "enabled" : "disabled";
	const char *rxen = (ctrl & CTRL_RXEN) ?
				   "enabled (forcibly receiving byte)" :
				   "disabled (receiving only when /CS low)";

	static const char *rx_intr_mode_str[] = {
		[CTRL_RX_INT_IRQ_ONE_BYTE]    = "1 byte",
		[CTRL_RX_INT_IRQ_TWO_BYTES]   = "2 bytes",
		[CTRL_RX_INT_IRQ_FOUR_BYTES]  = "4 bytes",
		[CTRL_RX_INT_IRQ_EIGHT_BYTES] = "8 bytes"
	};

	uint rx_intr_mode = (ctrl >> CTRL_RX_INT_MODE_SHIFT) &
			    CTRL_RX_INT_MODE_MASK;

	const char *tx_intr  = (ctrl & CTRL_TX_INT_EN) ? "enabled" : "disabled";
	const char *rx_intr  = (ctrl & CTRL_RX_INT_EN) ? "enabled" : "disabled";
	const char *dsr_intr = (ctrl & CTRL_DSR_INT_EN) ? "enabled" :
							  "disabled";
	const char *port_sel = (ctrl & CTRL_PORT_SEL) ? "port 2" : "port 1";

	LOG_INFO(
		ctx,
		"ctrl set to 0x%04X (txen = %s, dtr output level = %s, rxen = "
		"%s, rx interrupt mode = %s, tx interrupt enable = %s, rx "
		"interrupt enable = %s, dsr interrupt enable = %s, port select "
		"= %s)",
		ctrl, txen, dtr, rxen, rx_intr_mode_str[rx_intr_mode], tx_intr,
		rx_intr, dsr_intr, port_sel);

	if (bits_became_set(ctx->sio0.ctrl, ctrl, CTRL_DTR_OUT_LVL)) {
		LOG_DBG(ctx, "DTR output level set high - CS has gone low");
		reset_peripherals(ctx);
	} else if (bits_became_clr(ctx->sio0.ctrl, ctrl, CTRL_DTR_OUT_LVL)) {
		LOG_DBG(ctx, "DTR output level set low - CS has gone high");
		ctx->sio0.curr_dev = NULL;
	}

	ctx->sio0.ctrl = ctrl;
}

void p_sio0_baud_set(struct p_ctx *ctx, u16 baud)
{
	uint bps = calc_baud(ctx, baud);

	LOG_INFO(ctx, "baud reload value set to %d (new baud rate = %u bps)",
		 baud, bps);

	ctx->sio0.baud = baud;
}

void p_attach_dev_to_sio0(struct p_ctx *ctx, struct p_sio0_dev *dev,
			  enum sio0_slot slot)
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
