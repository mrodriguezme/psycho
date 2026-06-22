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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "psycho/compiler.h"
#include "psycho/types.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define swap(a, b)                       \
	({                               \
		__auto_type _tmp = *(a); \
		*(a) = *(b);             \
		*(b) = _tmp;             \
	})

#define static_assert_offset(x, memb, off) \
	_Static_assert(offsetof(x, memb) == (off), "Offset is not correct.")

#define ZEXT_FUNC(from, to)                                   \
	P_NODISCARD P_ALWAYS_INLINE u##to zext_##from##_##to( \
		const u##from val)                            \
	{                                                     \
		return val;                                   \
	}

#define SEXT_FUNC(from, to)                                   \
	P_NODISCARD P_ALWAYS_INLINE u##to sext_##from##_##to( \
		const u##from val)                            \
	{                                                     \
		return (s##from)val;                          \
	}

ZEXT_FUNC(8, 32);
ZEXT_FUNC(16, 32);
ZEXT_FUNC(32, 64);
SEXT_FUNC(8, 32);
SEXT_FUNC(16, 32);
SEXT_FUNC(32, 64);
