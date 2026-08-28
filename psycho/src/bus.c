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
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "gpu.h"
#include "intctrl.h"
#include "log.h"
#include "sio0.h"

LOG_MOD(P_LOG_BUS);

#define BIOS_BEGIN (0x1FC00000)
#define BIOS_END   (0x1FC7FFFF)
#define BIOS_MASK  (0x000FFFFF)

#define RAM_BEGIN  (0x00000000)
#define RAM_END	   (0x001FFFFF)
#define RAM_MASK   (0x00FFFFFF)

#define SPAD_BEGIN (0x1F800000)
#define SPAD_END   (0x1F8003FF)
#define SPAD_MASK  (0x00000FFF)

u8 *p_bios_data_get(struct p_ctx *ctx)
{
	return ctx->bus.bios;
}

void p_bus_init(struct p_ctx *ctx)
{
	ctx->bus.ram = malloc(RAM_END + 1);
}

u32 p_load32(struct p_ctx *ctx, u32 paddr)
{
	u32 ret;

	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		memcpy(&ret, &ctx->bus.ram[paddr & RAM_MASK], sizeof(u32));
		return ret;

	case SPAD_BEGIN ... SPAD_END:
		memcpy(&ret, &ctx->bus.spad[paddr & SPAD_MASK], sizeof(u32));
		return ret;

	case I_STAT:
		return ctx->intctrl.i_stat;

	case I_MASK:
		return ctx->intctrl.i_mask;

	case GPU_GPUSTAT:
		return ctx->gpu.gpustat;

	case GPU_GPUREAD:
		return p_gpuread(ctx);

	case BIOS_BEGIN ... BIOS_END:
		memcpy(&ret, &ctx->bus.bios[paddr & BIOS_MASK], sizeof(u32));
		return ret;

	default:
		LOG_WARN(ctx, "bad load32 0x%08X; returning 0xFFFFFFFF", paddr);
		return UINT32_MAX;
	}
}

u16 p_load16(struct p_ctx *ctx, u32 paddr)
{
	u16 ret;

	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		memcpy(&ret, &ctx->bus.ram[paddr & RAM_MASK], sizeof(u16));
		return ret;

	case SPAD_BEGIN ... SPAD_END:
		memcpy(&ret, &ctx->bus.spad[paddr & SPAD_MASK], sizeof(u16));
		return ret;

	case SIO0_CTRL:
		return ctx->sio0.ctrl;

	default:
		LOG_WARN(ctx, "bad load16 0x%08X; returning 0xFFFF", paddr);
		return UINT16_MAX;
	}
}

u8 p_load8(struct p_ctx *ctx, u32 paddr)
{
	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		return ctx->bus.ram[paddr & RAM_MASK];

	case SPAD_BEGIN ... SPAD_END:
		return ctx->bus.spad[paddr & SPAD_MASK];

	case BIOS_BEGIN ... BIOS_END:
		return ctx->bus.bios[paddr & BIOS_MASK];

	case SIO0_RX_DATA:
		return p_sio0_rx_pop8(ctx);

	default:
		LOG_WARN(ctx, "bad load8 0x%08X; returning 0xFF", paddr);
		return UINT8_MAX;
	}
}

void p_store32(struct p_ctx *ctx, u32 paddr, u32 data)
{
	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		memcpy(&ctx->bus.ram[paddr & RAM_MASK], &data, sizeof(u32));
		return;

	case SPAD_BEGIN ... SPAD_END:
		memcpy(&ctx->bus.spad[paddr & SPAD_MASK], &data, sizeof(u32));
		return;

	case I_STAT:
		p_irq_ack(ctx, data);
		return;

	case I_MASK:
		p_irq_mask_set(ctx, data);
		return;

	case GPU_GP0:
		p_gp0(ctx, data);
		return;

	case GPU_GP1:
		p_gp1(ctx, data);
		return;

	default:
		break;
	}

	LOG_WARN(ctx, "bad store32 0x%08X <- 0x%08X; ignoring", paddr, data);
}

void p_store16(struct p_ctx *ctx, u32 paddr, u16 data)
{
	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		memcpy(&ctx->bus.ram[paddr & RAM_MASK], &data, sizeof(u16));
		return;

	case SPAD_BEGIN ... SPAD_END:
		memcpy(&ctx->bus.spad[paddr & SPAD_MASK], &data, sizeof(u16));
		return;

	case SIO0_CTRL:
		p_sio0_ctrl_set(ctx, data);
		return;

	default:
		break;
	}

	LOG_WARN(ctx, "bad store16 0x%08X <- 0x%04X; ignoring", paddr, data);
}

void p_store8(struct p_ctx *ctx, u32 paddr, u8 data)
{
	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		if (paddr == 0x0004d474) {
			//ctx->bus.ram[paddr] = 1;
			//return;
		}
		ctx->bus.ram[paddr & RAM_MASK] = data;
		return;

	case SPAD_BEGIN ... SPAD_END:
		ctx->bus.spad[paddr & SPAD_MASK] = data;
		return;

	case SIO0_TX_DATA:
		p_sio0_tx(ctx, data);
		return;

	default:
		break;
	}

	LOG_WARN(ctx, "bad store8 0x%08X <- 0x%02X; ignoring", paddr, data);
}

void *p_get_mem_area(struct p_ctx *ctx, const u32 paddr)
{
	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		return &ctx->bus.ram[paddr & RAM_MASK];

	case BIOS_BEGIN ... BIOS_END:
		return &ctx->bus.bios[paddr & BIOS_MASK];

	default:
		return NULL;
	}
}
