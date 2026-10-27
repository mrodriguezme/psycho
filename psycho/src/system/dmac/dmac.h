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

#pragma once

#include "psycho/ctx.h"

#define DMAC_MADR(channel) ((0x1F801080) + ((channel) * 0x10))
#define DMAC_BCR(channel)  ((0x1F801084) + ((channel) * 0x10))
#define DMAC_CHCR(channel) ((0x1F801088) + ((channel) * 0x10))

enum dmac_channel {
	DMAC_CHANNEL_MDECIN	= 0,
	DMAC_CHANNEL_MDECOUT	= 1,
	DMAC_CHANNEL_GPU	= 2,
	DMAC_CHANNEL_CDROM	= 3,
	DMAC_CHANNEL_SPU	= 4,
	DMAC_CHANNEL_PIO	= 5,
	DMAC_CHANNEL_OTC	= 6
};

enum {
	DMAC_MDECIN_MADR  = DMAC_MADR(0),
	DMAC_MDECIN_BCR	  = DMAC_BCR(0),
	DMAC_MDECIN_CHCR  = DMAC_CHCR(0),
	DMAC_MDECOUT_MADR = DMAC_MADR(1),
	DMAC_MDECOUT_BCR  = DMAC_BCR(1),
	DMAC_MDECOUT_CHCR = DMAC_CHCR(1),
	DMAC_GPU_MADR	  = DMAC_MADR(2),
	DMAC_GPU_BCR	  = DMAC_BCR(2),
	DMAC_GPU_CHCR	  = DMAC_CHCR(2),
	DMAC_CDROM_MADR	  = DMAC_MADR(3),
	DMAC_CDROM_BCR	  = DMAC_BCR(3),
	DMAC_CDROM_CHCR	  = DMAC_CHCR(3),
	DMAC_SPU_MADR	  = DMAC_MADR(4),
	DMAC_SPU_BCR	  = DMAC_BCR(4),
	DMAC_SPU_CHCR	  = DMAC_CHCR(4),
	DMAC_PIO_MADR	  = DMAC_MADR(5),
	DMAC_PIO_BCR	  = DMAC_BCR(5),
	DMAC_PIO_CHCR	  = DMAC_CHCR(5),
	DMAC_OTC_MADR	  = DMAC_MADR(6),
	DMAC_OTC_BCR	  = DMAC_BCR(6),
	DMAC_OTC_CHCR	  = DMAC_CHCR(6),
	DMAC_DPCR	  = 0x1F8010F0,
	DMAC_DICR	  = 0x1F8010F4
};

void psycho_dmac_reset(struct psycho_ctx *ctx);

void dmac_channel_madr_write(struct psycho_ctx *ctx, enum dmac_channel channel, uint32_t madr);
void dmac_channel_bcr_write(struct psycho_ctx *ctx, enum dmac_channel channel, uint32_t bcr);
void dmac_channel_chcr_write(struct psycho_ctx *ctx, enum dmac_channel channel, uint32_t word);

void dmac_dpcr_write(struct psycho_ctx *ctx, uint32_t word) PSYCHO_NONNULL;
void dmac_dicr_write(struct psycho_ctx *ctx, uint32_t word) PSYCHO_NONNULL;

void psycho_dmac_channel_dreq_update(struct psycho_ctx *ctx, enum dmac_channel channel, bool asserted);
