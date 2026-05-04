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

#pragma once

#include <stdint.h>
#include "psycho/compiler.h"
#include "types.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ZEXT_FUNC(width)                                                 \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE u32 zero_ext_##width##_32( \
		const u##width val)                                      \
	{                                                                \
		return val;                                              \
	}

#define SEXT_FUNC(width)                                                 \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE u32 sign_ext_##width##_32( \
		const u##width val)                                      \
	{                                                                \
		return (s##width)val;                                    \
	}

ZEXT_FUNC(8);
ZEXT_FUNC(16);
SEXT_FUNC(8);
SEXT_FUNC(16);
