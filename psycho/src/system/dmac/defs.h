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

#include "common/util.h"

// ---------------------------------------------------------------------------
// DMAC channel MADR register layout
// ---------------------------------------------------------------------------
constexpr uint32_t MADR_MASK = GENMASK32(23, 0);

// ---------------------------------------------------------------------------
// DMAC channel BCR register layout
// ---------------------------------------------------------------------------
constexpr uint32_t BC_BS_MASK = GENMASK32(15, 0);
constexpr uint32_t BA_MASK    = GENMASK32(31, 16);

// ---------------------------------------------------------------------------
// DMAC channel CHCR register layout
// ---------------------------------------------------------------------------
constexpr uint32_t CHCR_BUS_SNOOPING		    = 1 << 30;
constexpr uint32_t CHCR_PAUSE_TRANSFER_DURING_BURST = 1 << 29;
constexpr uint32_t CHCR_FORCE_TRANSFER_WITHOUT_DREQ = 1 << 28;
constexpr uint32_t CHCR_START_TRANSFER		    = 1 << 24;
constexpr uint32_t CHCR_CHOPPING_CPU_WINDOW_SIZE    = GENMASK32(22, 20);
constexpr uint32_t CHCR_CHOPPING_DMA_WINDOW_SIZE    = GENMASK32(18, 16);
constexpr uint32_t CHCR_SYNCMODE_MASK		    = GENMASK32(10, 9);
constexpr uint32_t CHCR_BIT8			    = 1 << 8;
constexpr uint32_t CHCR_MADR_PER_STEP		    = 1 << 1;
constexpr uint32_t CHCR_TRANSFER_DIR		    = 1 << 0;

// ---------------------------------------------------------------------------
// DPCR register layout
// ---------------------------------------------------------------------------
constexpr uint32_t DPCR_MEM_ACCESS_PRIORITY   = GENMASK32(30, 28);
constexpr uint32_t DPCR_OTC_MASTER_ENABLE     = 1 << 27;
constexpr uint32_t DPCR_OTC_PRIORITY	      = GENMASK32(26, 24);
constexpr uint32_t DPCR_PIO_MASTER_ENABLE     = 1 << 23;
constexpr uint32_t DPCR_PIO_PRIORITY	      = GENMASK32(22, 20);
constexpr uint32_t DPCR_SPU_MASTER_ENABLE     = 1 << 19;
constexpr uint32_t DPCR_SPU_PRIORITY	      = GENMASK32(18, 16);
constexpr uint32_t DPCR_CDROM_MASTER_ENABLE   = 1 << 15;
constexpr uint32_t DPCR_CDROM_PRIORITY	      = GENMASK32(14, 12);
constexpr uint32_t DPCR_GPU_MASTER_ENABLE     = 1 << 11;
constexpr uint32_t DPCR_GPU_PRIORITY	      = GENMASK32(10, 8);
constexpr uint32_t DPCR_MDECOUT_MASTER_ENABLE = 1 << 7;
constexpr uint32_t DPCR_MDECOUT_PRIORITY      = GENMASK32(6, 4);
constexpr uint32_t DPCR_MDECIN_MASTER_ENABLE  = 1 << 3;
constexpr uint32_t DPCR_MDECIN_PRIORITY	      = GENMASK32(2, 0);

// ---------------------------------------------------------------------------
// DICR register layout
// ---------------------------------------------------------------------------
constexpr uint32_t DICR_MASTER_INTERRUPT_FLAG	= UINT32_C(1) << 31;
constexpr uint32_t DICR_CHANNEL_INTERRUPT_FLAGS = GENMASK32(30, 24);
constexpr uint32_t DICR_MASTER_INTERRUPT_ENABLE = 1 << 23;
constexpr uint32_t DICR_CHANNEL_INTERRUPT_MASK	= GENMASK32(22, 16);
constexpr uint32_t DICR_BUS_ERR_FLAG		= 1 << 15;
constexpr uint32_t DICR_CHANNEL_COMPLETION_MASK = GENMASK32(6, 0);
