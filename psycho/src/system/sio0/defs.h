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
// SIO0_SR register layout
// ---------------------------------------------------------------------------
constexpr uint32_t SR_TXRDY	   = 1 << 0;
constexpr uint32_t SR_RXRDY	   = 1 << 1;
constexpr uint32_t SR_TXU	   = 1 << 2;
constexpr uint32_t SR_PERROR	   = 1 << 3;
constexpr uint32_t SR_DSR	   = 1 << 7;
constexpr uint32_t SR_IRQ	   = 1 << 9;
constexpr uint32_t SR_BAUDTMR_MASK = GENMASK32(31, 11);
constexpr uint32_t SR_BITS	   = SR_TXRDY | SR_RXRDY | SR_TXU | SR_PERROR | SR_DSR | SR_IRQ | SR_BAUDTMR_MASK;

// ---------------------------------------------------------------------------
// SIO0_MR register layout
// ---------------------------------------------------------------------------
constexpr uint16_t MR_BR_MASK	= GENMASK32(1, 0);
constexpr uint16_t MR_BR_MUL1_0 = 0;
constexpr uint16_t MR_BR_MUL1_1 = 1;
constexpr uint16_t MR_BR_MUL16	= 2;
constexpr uint16_t MR_BR_MUL64	= 3;

constexpr uint16_t MR_CHLEN_MASK = GENMASK32(3, 2);
constexpr uint16_t MR_CHLEN_5BIT = 0;
constexpr uint16_t MR_CHLEN_6BIT = 1;
constexpr uint16_t MR_CHLEN_7BIT = 2;
constexpr uint16_t MR_CHLEN_8BIT = 3;

constexpr uint16_t MR_PEN  = 1 << 4;
constexpr uint16_t MR_P	   = 1 << 5;
constexpr uint16_t MR_CPOL = 1 << 8;

constexpr uint16_t MR_BITS = MR_BR_MASK | MR_CHLEN_MASK | MR_PEN | MR_P | MR_CPOL;

// ---------------------------------------------------------------------------
// SIO0_CR register layout
// ---------------------------------------------------------------------------
constexpr uint16_t CR_TXEN   = 1 << 0;
constexpr uint16_t CR_DTR    = 1 << 1;
constexpr uint16_t CR_RXEN   = 1 << 2;
constexpr uint16_t CR_ERRRST = 1 << 4;
constexpr uint16_t CR_INTRST = 1 << 6;

constexpr uint16_t CR_BUFSZ_MASK = GENMASK32(9, 8);
constexpr uint16_t CR_BUFSZ_1	 = 0;
constexpr uint16_t CR_BUFSZ_2	 = 1;
constexpr uint16_t CR_BUFSZ_4	 = 2;
constexpr uint16_t CR_BUFSZ_8	 = 3;

constexpr uint16_t CR_TXIEN  = 1 << 10;
constexpr uint16_t CR_RXIEN  = 1 << 11;
constexpr uint16_t CR_DSRIEN = 1 << 12;
constexpr uint16_t CR_PORT   = 1 << 13;

constexpr uint16_t CR_BITS = CR_TXEN | CR_DTR | CR_RXEN | CR_ERRRST | CR_INTRST | CR_BUFSZ_MASK | CR_TXIEN | CR_RXIEN |
			     CR_DSRIEN | CR_PORT;

// ---------------------------------------------------------------------------
// SIO0 bus signal levels
// ---------------------------------------------------------------------------
constexpr uint8_t HI_Z = UINT8_MAX;
