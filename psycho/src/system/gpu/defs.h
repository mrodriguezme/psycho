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

enum dma_direction {
	DMD_OFF	 = 0,
	DMD_WFNF = 1,
	DMD_WFEP = 2,
	DMD_RFFL = 3,
};

enum {
	CMD_TYPE_MISC		   = 0,
	CMD_TYPE_POLY		   = 1,
	CMD_TYPE_LINE		   = 2,
	CMD_TYPE_RECT		   = 3,
	CMD_TYPE_VRAM_TO_VRAM_BLIT = 4,
	CMD_TYPE_CPU_TO_VRAM_BLIT  = 5,
	CMD_TYPE_VRAM_TO_CPU_BLIT  = 6,
	CMD_TYPE_ENV		   = 7
};

enum {
	GP1_CMD_RESET			     = 0x00,
	GP1_CMD_COMMAND_BUFFER_RESET	     = 0x01,
	GP1_CMD_DISPLAY_DISABLED_SET	     = 0x03,
	GP1_CMD_DMA_DIRECTION_SET	     = 0x04,
	GP1_CMD_DISPLAY_AREA_START_SET	     = 0x05,
	GP1_CMD_DISPLAY_RANGE_HORIZONTAL_SET = 0x06,
	GP1_CMD_DISPLAY_RANGE_VERTICAL_SET   = 0x07,
	GP1_CMD_DISPLAY_MODE_SET	     = 0x08
};

// ---------------------------------------------------------------------------
// GP0 environment type commands
// ---------------------------------------------------------------------------
enum {
	ENV_CMD_DRAW_MODE_SET		      = 0xE1,
	ENV_CMD_DRAWING_AREA_TOP_LEFT_SET     = 0xE3,
	ENV_CMD_DRAWING_AREA_BOTTOM_RIGHT_SET = 0xE4,
	ENV_CMD_DRAWING_OFFSET_SET	      = 0xE5,
	ENV_CMD_MASK_BIT_SET		      = 0xE6
};

enum {
	MISC_CMD_NOP	   = 0x00,
	MISC_CMD_FILL_VRAM = 0x02,
};

// ---------------------------------------------------------------------------
// Packet header information
// ---------------------------------------------------------------------------
constexpr unsigned int PACKET_CMD_TYPE_SHIFT = 29;
constexpr unsigned int PACKET_CMD_SHIFT	     = 24;
constexpr unsigned int PACKET_PARAMS_MASK    = GENMASK32(23, 0);

// ---------------------------------------------------------------------------
// STAT bits
// ---------------------------------------------------------------------------
constexpr uint32_t STAT_TBX_MASK = GENMASK32(3, 0);
constexpr uint32_t STAT_TBY	 = 1 << 4;
constexpr uint32_t STAT_ABR_MASK = GENMASK32(6, 5);
constexpr uint32_t STAT_TPF_MASK = GENMASK32(8, 7);
constexpr uint32_t STAT_DTD	 = 1 << 9;
constexpr uint32_t STAT_DFE	 = 1 << 10;
constexpr uint32_t STAT_PBW	 = 1 << 11;
constexpr uint32_t STAT_PBC	 = 1 << 12;
constexpr uint32_t STAT_ODE2	 = 1 << 13;
constexpr uint32_t STAT_REV	 = 1 << 14;
constexpr uint32_t STAT_TBY2	 = 1 << 15;
constexpr uint32_t STAT_HDS2	 = 1 << 16;
constexpr uint32_t STAT_HDS_MASK = GENMASK32(18, 17);
constexpr uint32_t STAT_VDS	 = 1 << 19;
constexpr uint32_t STAT_NPB	 = 1 << 20;
constexpr uint32_t STAT_LBS	 = 1 << 21;
constexpr uint32_t STAT_IRS	 = 1 << 22;
constexpr uint32_t STAT_DMSK	 = 1 << 23;
constexpr uint32_t STAT_IRQ	 = 1 << 24;
constexpr uint32_t STAT_DREQ	 = 1 << 25;
constexpr uint32_t STAT_IDLE	 = 1 << 26;
constexpr uint32_t STAT_RFFL	 = 1 << 27;
constexpr uint32_t STAT_WFEP	 = 1 << 28;
constexpr uint32_t STAT_DMD_MASK = GENMASK32(30, 29);
constexpr uint32_t STAT_DMD_OFF	 = 0;
constexpr uint32_t STAT_DMD_WFNF = 1;
constexpr uint32_t STAT_DMD_WFEP = 2;
constexpr uint32_t STAT_DMD_RFFL = 3;
constexpr uint32_t STAT_ODE	 = UINT32_C(1) << 31;

// ---------------------------------------------------------------------------
// GP0 draw polygon command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t POLY_R0_MASK = GENMASK32(7, 0);
constexpr uint32_t POLY_G0_MASK = GENMASK32(15, 8);
constexpr uint32_t POLY_B0_MASK = GENMASK32(23, 16);
constexpr uint32_t POLY_TGE	= 1 << 24;
constexpr uint32_t POLY_ABE	= 1 << 25;
constexpr uint32_t POLY_TME	= 1 << 26;
constexpr uint32_t POLY_VTX	= 1 << 27;
constexpr uint32_t POLY_IIP	= 1 << 28;

// ---------------------------------------------------------------------------
// GP0 draw rectangle command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t RECT_R0_MASK = GENMASK32(7, 0);
constexpr uint32_t RECT_G0_MASK = GENMASK32(15, 8);
constexpr uint32_t RECT_B0_MASK = GENMASK32(23, 16);
constexpr uint32_t RECT_TGE	= 1 << 24;
constexpr uint32_t RECT_ABE	= 1 << 25;
constexpr uint32_t RECT_TME	= 1 << 26;

// ---------------------------------------------------------------------------
// "GP0(0xE3) - Set Drawing Area top left (X1,Y1)" command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t DRAWING_AREA_MASK_X1	      = GENMASK32(9, 0);
constexpr uint32_t DRAWING_AREA_MASK_Y1	      = GENMASK32(18, 10);
constexpr uint32_t DRAWING_AREA_MASK_TOP_LEFT = GENMASK32(18, 0);

// ---------------------------------------------------------------------------
// "GP0(0xE4) - Set Drawing Area bottom right (X2,Y2)" command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t DRAWING_AREA_MASK_X2		  = GENMASK32(9, 0);
constexpr uint32_t DRAWING_AREA_MASK_Y2		  = GENMASK32(18, 10);
constexpr uint32_t DRAWING_AREA_MASK_BOTTOM_RIGHT = GENMASK32(18, 0);

// ---------------------------------------------------------------------------
// "GP0(0xE5) - Set Drawing Offset (X,Y)" command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t DRAWING_OFFSET_X_MASK = GENMASK32(10, 0);
constexpr uint32_t DRAWING_OFFSET_Y_MASK = GENMASK32(21, 11);
constexpr uint32_t DRAWING_OFFSET_MASK	 = GENMASK32(21, 0);

// ---------------------------------------------------------------------------
// "GP0(0xE6) - Mask Bit Setting" command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t MASK_BIT_SETTING_PBW = 1 << 0;
constexpr uint32_t MASK_BIT_SETTING_PBC = 1 << 1;

// ---------------------------------------------------------------------------
// "GP1(0x05) - Start of Display area (in VRAM)" command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t DISPLAY_AREA_START_MASK   = GENMASK32(18, 0);
constexpr uint32_t DISPLAY_AREA_START_X_MASK = GENMASK32(9, 0);
constexpr uint32_t DISPLAY_AREA_START_Y_MASK = GENMASK32(18, 10);

// ---------------------------------------------------------------------------
// "GP1(0x06) - Horizontal Display range (on Screen)" command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t DISPLAY_RANGE_HORIZONTAL_X1_MASK = GENMASK32(11, 0);
constexpr uint32_t DISPLAY_RANGE_HORIZONTAL_X2_MASK = GENMASK32(23, 12);
constexpr uint32_t DISPLAY_RANGE_HORIZONTAL_MASK    = GENMASK32(23, 0);

// ---------------------------------------------------------------------------
// "GP1(07h) - Vertical Display range (on Screen)" command parameters
// ---------------------------------------------------------------------------
constexpr uint32_t DISPLAY_RANGE_VERTICAL_Y1_MASK = GENMASK32(9, 0);
constexpr uint32_t DISPLAY_RANGE_VERTICAL_Y2_MASK = GENMASK32(19, 10);
constexpr uint32_t DISPLAY_RANGE_VERTICAL_MASK	  = GENMASK32(19, 0);

// ---------------------------------------------------------------------------
// "GP1(08h) - Display mode" command parameters and corresponding STAT flags
// ---------------------------------------------------------------------------
constexpr uint32_t DISPLAY_MODE_HDS_MASK  = GENMASK32(1, 0);
constexpr uint32_t DISPLAY_MODE_VDS	  = 1 << 2;
constexpr uint32_t DISPLAY_MODE_NPB	  = 1 << 3;
constexpr uint32_t DISPLAY_MODE_LBS	  = 1 << 4;
constexpr uint32_t DISPLAY_MODE_IRS	  = 1 << 5;
constexpr uint32_t DISPLAY_MODE_HDS2	  = 1 << 6;
constexpr uint32_t DISPLAY_MODE_REV	  = 1 << 7;
constexpr uint32_t STAT_DISPLAY_MODE_MASK = STAT_HDS_MASK | STAT_VDS | STAT_NPB | STAT_LBS | STAT_IRS | STAT_HDS2 |
					    STAT_REV;

// ---------------------------------------------------------------------------
// Reset values of registers after GPU reset via GP1(0x01)
// ---------------------------------------------------------------------------

// x1 = 0x200, x2 = 0x200 + (256 * 10)
constexpr uint32_t DISPLAY_RANGE_HORIZONTAL_RESET_VALUE = 0x00C00200;

// y1 = 0x010, y2 = 0x010 + 240
constexpr uint32_t DISPLAY_RANGE_VERTICAL_RESET_VALUE = 0x00040010;

constexpr uint32_t DISPLAY_MODE_RESET_VALUE	  = 0;
constexpr uint32_t DISPLAY_AREA_START_RESET_VALUE = 0;
constexpr uint32_t DMA_DIRECTION_RESET_VALUE	  = DMD_OFF;
constexpr bool DISPLAY_DISABLED_RESET_VALUE	  = true;

constexpr uint32_t DRAW_MODE_TBY2 = 1 << 11;

// Command bits 0-10 map directly onto GPUSTAT bits 0-10.
constexpr uint32_t STAT_DRAW_MODE_DIRECT_MASK = STAT_TBX_MASK | STAT_TBY | STAT_ABR_MASK | STAT_TPF_MASK | STAT_DTD |
						STAT_DFE;
constexpr uint32_t STAT_DRAW_MODE_MASK	      = STAT_DRAW_MODE_DIRECT_MASK | STAT_TBY2;

extern const int8_t psycho_gpu_dither_matrix[4][4];
