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

#include "debug/log.h"

#include "gpu/gpu.h"

#include "bus.h"
#include "intc.h"
#include "sio0.h"

DECLARE_LOG_MODULE(PSYCHO_LOG_MODULE_BUS);

enum {
	BIOS_BEGIN = 0x1FC00000,
	BIOS_END   = 0x1FC7FFFF,
	BIOS_MASK  = 0x000FFFFF,

	RAM_BEGIN = 0x00000000,
	RAM_END	  = PSYCHO_BUS_RAM_SIZE_MAX_BYTES,
	RAM_MASK  = 0x001FFFFF,

	SCRATCHPAD_BEGIN = 0x1F800000,
	SCRATCHPAD_END	 = 0x1F8003FF,
	SCRATCHPAD_MASK	 = 0x00000FFF
};

enum {
	DEV0_BASE_ADDR	= 0x1F801000,
	DEV8_BASE_ADDR	= 0x1F801004,
	DEV0_DELAY_SIZE = 0x1F801008,
	DEV1_DELAY_SIZE = 0x1F80100C,
	DEV2_DELAY_SIZE = 0x1F801010,
	DEV4_DELAY_SIZE = 0x1F801014,
	DEV5_DELAY_SIZE = 0x1F801018,
	DEV8_DELAY_SIZE = 0x1F80101C,
	COM_DELAY	= 0x1F801020,
	DRAM_CTRL	= 0x1F801060
};

#define BIU_CACHE_CTRL_REG (UINT32_C(0xFFFE0130))

uint8_t *p_bios_data_get(struct psycho_ctx *ctx)
{
	return ctx->bus.bios;
}

uint32_t psycho_bus_load_word(struct psycho_ctx *ctx, uint32_t paddr, bool *valid)
{
	uint32_t ret;

	*valid = true;

	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		memcpy(&ret, &ctx->bus.ram[paddr & RAM_MASK], sizeof(uint32_t));
		return ret;

	case SCRATCHPAD_BEGIN ... SCRATCHPAD_END:
		memcpy(&ret, &ctx->bus.spad[paddr & SCRATCHPAD_MASK], sizeof(uint32_t));
		return ret;

	case I_STAT:
		return ctx->intc.i_stat;

	case I_MASK:
		return ctx->intc.i_mask;

	case GPU_GPUSTAT:
		return ctx->gpu.gpustat;

	case GPU_GPUREAD:
		return p_gpuread(ctx);

	case BIOS_BEGIN ... BIOS_END:
		memcpy(&ret, &ctx->bus.bios[paddr & BIOS_MASK], sizeof(uint32_t));
		return ret;

	default:
		LOG_ERR(ctx, "bad word load from unknown physical address 0x%08X", paddr);
		*valid = false;

		return UINT32_MAX;
	}
}

uint16_t psycho_bus_load_halfword(struct psycho_ctx *ctx, uint32_t paddr, bool *valid)
{
	uint16_t ret;

	*valid = true;

	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		memcpy(&ret, &ctx->bus.ram[paddr & RAM_MASK], sizeof(uint16_t));
		return ret;

	case SCRATCHPAD_BEGIN ... SCRATCHPAD_END:
		memcpy(&ret, &ctx->bus.spad[paddr & SCRATCHPAD_MASK], sizeof(uint16_t));
		return ret;

	case SIO0_CR:
		return ctx->sio0.cr;

	case 0x1F801120:
		return UINT16_MAX;

	default:
		LOG_ERR(ctx, "bad halfword load from unknown physical address 0x%08X", paddr);
		*valid = false;

		return UINT16_MAX;
	}
}

uint8_t psycho_bus_load_byte(struct psycho_ctx *ctx, uint32_t paddr, bool *valid)
{
	*valid = true;

	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		return ctx->bus.ram[paddr & RAM_MASK];

	case SCRATCHPAD_BEGIN ... SCRATCHPAD_END:
		return ctx->bus.spad[paddr & SCRATCHPAD_MASK];

	case BIOS_BEGIN ... BIOS_END:
		return ctx->bus.bios[paddr & BIOS_MASK];

	case SIO0_DR:
		return psycho_sio0_dr_read_byte(ctx);

	case 0x1F000084:
		return UINT8_MAX;

	default:
		LOG_ERR(ctx, "bad byte load from unknown physical address 0x%08X", paddr);
		*valid = false;

		return UINT8_MAX;
	}
}

bool psycho_bus_store_word(struct psycho_ctx *ctx, uint32_t paddr, uint32_t data)
{
	bool valid = true;

	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		memcpy(&ctx->bus.ram[paddr & RAM_MASK], &data, sizeof(uint32_t));
		break;

	case SCRATCHPAD_BEGIN ... SCRATCHPAD_END:
		memcpy(&ctx->bus.spad[paddr & SCRATCHPAD_MASK], &data, sizeof(uint32_t));
		break;

	case I_STAT:
		psycho_intc_irqs_ack(ctx, data);
		break;

	case I_MASK:
		psycho_intc_mask_set(ctx, data);
		break;

	case GPU_GP0:
		p_gp0(ctx, data);
		break;

	case GPU_GP1:
		p_gp1(ctx, data);
		break;

	case DEV0_BASE_ADDR:
	case DEV8_BASE_ADDR:
	case DEV0_DELAY_SIZE:
	case DEV1_DELAY_SIZE:
	case DEV2_DELAY_SIZE:
	case DEV4_DELAY_SIZE:
	case DEV5_DELAY_SIZE:
	case DEV8_DELAY_SIZE:
	case COM_DELAY:
	case DRAM_CTRL:
	case BIU_CACHE_CTRL_REG:
		break;

	default:
		LOG_ERR(ctx, "bad word store to unknown physical address 0x%08X (data=0x%08X)", paddr, data);
		valid = false;

		break;
	}
	return valid;
}

bool psycho_bus_store_halfword(struct psycho_ctx *ctx, uint32_t paddr, uint16_t data)
{
	bool valid = true;

	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		memcpy(&ctx->bus.ram[paddr & RAM_MASK], &data, sizeof(uint16_t));
		break;

	case SCRATCHPAD_BEGIN ... SCRATCHPAD_END:
		memcpy(&ctx->bus.spad[paddr & SCRATCHPAD_MASK], &data, sizeof(uint16_t));
		break;

	case SIO0_MR:
		psycho_sio0_mr_write_halfword(ctx, data);
		break;

	case SIO0_CR:
		psycho_sio0_cr_write_halfword(ctx, data);
		break;

	case SIO0_BR:
		psycho_sio0_br_write_halfword(ctx, data);
		break;

	case 0x1F801D80:
	case 0x1F801D82:
	case 0x1F801D84:
	case 0x1F801D86:
	case 0x1F801104:
	case 0x1F801108:
	case 0x1F801100:
	case 0x1F801114:
	case 0x1F801118:
	case 0x1F801110:
	case 0x1F801124:
	case 0x1F801128:
	case 0x1F801120:
		break;

	default:
		LOG_ERR(ctx, "bad halfword store to unknown physical address 0x%08X (data=0x%04X)", paddr, data);
		valid = false;

		break;
	}
	return valid;
}

bool psycho_bus_store_byte(struct psycho_ctx *ctx, uint32_t paddr, uint8_t data)
{
	bool valid = true;

	switch (paddr) {
	case RAM_BEGIN ... RAM_END:
		ctx->bus.ram[paddr & RAM_MASK] = data;
		break;

	case SCRATCHPAD_BEGIN ... SCRATCHPAD_END:
		ctx->bus.spad[paddr & SCRATCHPAD_MASK] = data;
		break;

	case SIO0_DR:
		psycho_sio0_dr_write_byte(ctx, data);
		break;

	case 0x1F802041:
		break;

	default:
		LOG_ERR(ctx, "bad byte store to unknown physical address 0x%08X (data=0x%02X)", paddr, data);
		valid = false;

		break;
	}
	return valid;
}

void *psycho_bus_get_mem_area_ptr(struct psycho_ctx *ctx, uint32_t paddr)
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
