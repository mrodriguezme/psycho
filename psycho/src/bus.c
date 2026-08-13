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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "gpu.h"
#include "intctrl.h"
#include "log.h"
#include "sio0.h"

LOG_MOD(P_LOG_BUS);

enum {
	// clang-format off

	BIOS_PADDR_BEGIN	= 0x1FC00000,
	BIOS_PADDR_END		= 0x1FC7FFFF,
	BIOS_PADDR_MASK		= 0x000FFFFF,

	RAM_PADDR_BEGIN	= 0x00000000,
	RAM_PADDR_END	= 0x001FFFFF,
	RAM_PADDR_MASK	= 0x00FFFFFF,

	SCRATCHPAD_PADDR_BEGIN	= 0x1F800000,
	SCRATCHPAD_PADDR_END	= 0x1F8003FF,
	SCRATCHPAD_PADDR_MASK	= 0x00000FFF

	// clang-format on
};

u8 *p_bios_data_get(struct p_ctx *const ctx)
{
	return ctx->bus.bios;
}

void p_bus_init(struct p_ctx *const ctx)
{
	ctx->bus.ram = malloc(RAM_PADDR_END + 1);
}

u32 p_load_word(struct p_ctx *const ctx, const u32 paddr)
{
	u32 word;

	switch (paddr) {
	case RAM_PADDR_BEGIN ... RAM_PADDR_END:
		memcpy(&word, &ctx->bus.ram[paddr & RAM_PADDR_MASK],
		       sizeof(u32));
		return word;

	case SCRATCHPAD_PADDR_BEGIN ... SCRATCHPAD_PADDR_END:
		memcpy(&word, &ctx->bus.spad[paddr & SCRATCHPAD_PADDR_MASK],
		       sizeof(u32));
		return word;

	case INTCTRL_ADDR_I_STAT:
		return ctx->intctrl.i_stat;

	case INTCTRL_ADDR_I_MASK:
		return ctx->intctrl.i_mask;

	case GPU_ADDR_GPUSTAT:
		return ctx->gpu.gpustat;

	case GPU_ADDR_GPUREAD:
		return p_gpu_gpuread(ctx);

	case BIOS_PADDR_BEGIN ... BIOS_PADDR_END:
		memcpy(&word, &ctx->bus.bios[paddr & BIOS_PADDR_MASK],
		       sizeof(u32));
		return word;

	case SIO0_STAT:
	case SIO0_MODE:
	case SIO0_CTRL:
	case SIO0_BAUD:
	case SIO0_RX_DATA:
		asm("nop");
		return 0;

	default:
		LOG_WARN(ctx,
			 "unknown word load: 0x%08X; returning 0xFFFF'FFFF",
			 paddr);
		return UINT32_MAX;
	}
}

u16 p_load_halfword(struct p_ctx *const ctx, const u32 paddr)
{
	u16 halfword;

	switch (paddr) {
	case RAM_PADDR_BEGIN ... RAM_PADDR_END:
		memcpy(&halfword, &ctx->bus.ram[paddr & RAM_PADDR_MASK],
		       sizeof(u16));
		return halfword;

	case SCRATCHPAD_PADDR_BEGIN ... SCRATCHPAD_PADDR_END:
		memcpy(&halfword, &ctx->bus.spad[paddr & SCRATCHPAD_PADDR_MASK],
		       sizeof(u16));
		return halfword;

	case SIO0_CTRL:
		return ctx->sio0.ctrl;

	default:
		LOG_WARN(ctx, "unknown halfword load: 0x%08X; returning 0xFFFF",
			 paddr);
		return UINT16_MAX;
	}
}

u8 p_load_byte(struct p_ctx *const ctx, const u32 paddr)
{
	switch (paddr) {
	case RAM_PADDR_BEGIN ... RAM_PADDR_END:
		return ctx->bus.ram[paddr & RAM_PADDR_MASK];

	case SCRATCHPAD_PADDR_BEGIN ... SCRATCHPAD_PADDR_END:
		return ctx->bus.spad[paddr & SCRATCHPAD_PADDR_MASK];

	case BIOS_PADDR_BEGIN ... BIOS_PADDR_END:
		return ctx->bus.bios[paddr & BIOS_PADDR_MASK];

	case SIO0_RX_DATA: {
		u8 data = p_sio0_rx_pop8(ctx);
		LOG_WARN(ctx, "rxdata -> 0x%02X", data);
		return data;
	}

	default:
		LOG_WARN(ctx, "unknown byte load: 0x%08X; returning 0xFF",
			 paddr);
		return UINT8_MAX;
	}
}

void p_store_word(struct p_ctx *const ctx, const u32 paddr, const u32 word)
{
	switch (paddr) {
	case RAM_PADDR_BEGIN ... RAM_PADDR_END:
		memcpy(&ctx->bus.ram[paddr & RAM_PADDR_MASK], &word,
		       sizeof(u32));
		return;

	case SCRATCHPAD_PADDR_BEGIN ... SCRATCHPAD_PADDR_END:
		memcpy(&ctx->bus.spad[paddr & SCRATCHPAD_PADDR_MASK], &word,
		       sizeof(u32));
		return;

	case INTCTRL_ADDR_I_STAT:
		p_irq_ack(ctx, word);
		return;

	case INTCTRL_ADDR_I_MASK:
		p_irq_mask_set(ctx, word);
		return;

	case GPU_ADDR_GP0:
		p_gpu_gp0(ctx, word);
		return;

	case GPU_ADDR_GP1:
		p_gpu_gp1(ctx, word);
		return;

	default:
		break;
	}

	LOG_WARN(ctx, "unknown word store: 0x%08X <- 0x%08X; ignoring", paddr,
		 word);
}

void p_store_halfword(struct p_ctx *const ctx, const u32 paddr,
		      const u16 halfword)
{
	switch (paddr) {
	case RAM_PADDR_BEGIN ... RAM_PADDR_END:
		memcpy(&ctx->bus.ram[paddr & RAM_PADDR_MASK], &halfword,
		       sizeof(u16));
		return;

	case SCRATCHPAD_PADDR_BEGIN ... SCRATCHPAD_PADDR_END:
		memcpy(&ctx->bus.spad[paddr & SCRATCHPAD_PADDR_MASK], &halfword,
		       sizeof(u16));
		return;

	case SIO0_CTRL:
		p_sio0_ctrl_set(ctx, halfword);
		return;

	case SIO0_BAUD:
		asm("nop");
		return;

	default:
		break;
	}
	LOG_WARN(ctx, "unknown halfword store: 0x%08X <- 0x%04X; ignoring",
		 paddr, halfword);
}

void p_store_byte(struct p_ctx *const ctx, u32 paddr, u8 byte)
{
	switch (paddr) {
	case RAM_PADDR_BEGIN ... RAM_PADDR_END:
		ctx->bus.ram[paddr & RAM_PADDR_MASK] = byte;
		return;

	case SCRATCHPAD_PADDR_BEGIN ... SCRATCHPAD_PADDR_END:
		ctx->bus.spad[paddr & SCRATCHPAD_PADDR_MASK] = byte;
		return;

	case SIO0_TX_DATA:
		LOG_INFO(ctx, "txdata <- 0x%02X", byte);

		p_sio0_tx(ctx, byte);
		return;

	default:
		break;
	}

	LOG_WARN(ctx, "unknown byte store: 0x%08X <- 0x%02X; ignoring", paddr,
		 byte);
}

void *p_get_mem_area(struct p_ctx *const ctx, const u32 paddr)
{
	switch (paddr) {
	case RAM_PADDR_BEGIN ... RAM_PADDR_END:
		return &ctx->bus.ram[paddr & RAM_PADDR_MASK];

	case BIOS_PADDR_BEGIN ... BIOS_PADDR_END:
		return &ctx->bus.bios[paddr & BIOS_PADDR_MASK];

	default:
		return NULL;
	}
}
