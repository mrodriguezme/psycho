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

#include <stdint.h>

constexpr uint32_t BIOS_BEGIN = 0x1FC00000;
constexpr uint32_t BIOS_END   = 0x1FC70000;
constexpr uint32_t BIOS_MASK  = 0x000FFFFF;

constexpr uint32_t RAM_BEGIN = 0x00000000;
constexpr uint32_t RAM_END   = PSYCHO_BUS_RAM_SIZE_MAX_BYTES;
constexpr uint32_t RAM_MASK  = 0x001FFFFF;

constexpr uint32_t SCRATCHPAD_BEGIN = 0x1F800000;
constexpr uint32_t SCRATCHPAD_END   = 0x1F8003FF;
constexpr uint32_t SCRATCHPAD_MASK  = 0x00000FFF;

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
