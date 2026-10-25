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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "psycho/system/cpu/cpu.h"
#include "psycho/common/compiler_support.h"

#define likely(x)		    __builtin_expect(!!(x), 1)
#define unlikely(x)		    __builtin_expect(!!(x), 0)

#define ARRAY_SIZE(x)		    (sizeof(x) / sizeof((x)[0]))
#define GENMASK32(hi, lo)	    (((UINT32_C(1) << ((hi) - (lo) + 1)) - 1) << (lo))

#define bits_became_set(a, b, bits) ((!((a) & (bits))) && ((b) & (bits)))
#define bits_became_clr(a, b, bits) (((a) & (bits)) && !((b) & (bits)))

#define max(a, b)                       \
	({                              \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a > _b ? _a : _b;      \
	})

#define min(a, b)                       \
	({                              \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a < _b ? _a : _b;      \
	})

#define swap(a, b)                       \
	({                               \
		__auto_type _tmp = *(a); \
		*(a)		 = *(b); \
		*(b)		 = _tmp; \
	})

#define clamp(val, min, max)                                      \
	({                                                        \
		__typeof__(val) _val = (val);                     \
		__typeof__(min) _min = (min);                     \
		__typeof__(max) _max = (max);                     \
		_val < _min ? _min : (_val > _max ? _max : _val); \
	})

PSYCHO_ALWAYS_INLINE uint32_t field_get32(uint32_t mask, uint32_t reg)
{
	return (reg & mask) >> __builtin_ctz(mask);
}

PSYCHO_ALWAYS_INLINE uint32_t field_prep32(uint32_t mask, uint32_t val)
{
	return (val << __builtin_ctz(mask)) & mask;
}

PSYCHO_ALWAYS_INLINE uint32_t field_set32(uint32_t src, uint32_t mask, uint32_t val)
{
	return (src & ~mask) | field_prep32(mask, val);
}

PSYCHO_ALWAYS_INLINE uint64_t us_to_cycles(uint64_t us)
{
	return (us * PSYCHO_CPU_CLKFREQ_HZ) / UINT64_C(1000000);
}

#define ZERO_EXTEND_FUNC(from, to)                                                                       \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint##to##_t zero_extend_##from##_##to(uint##from##_t val) \
	{                                                                                                \
		return val;                                                                              \
	}

#define SIGN_EXTEND_FUNC(from, to)                                                                       \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE uint##to##_t sign_extend_##from##_##to(uint##from##_t val) \
	{                                                                                                \
		return (int##from##_t)val;                                                               \
	}

ZERO_EXTEND_FUNC(8, 32);
ZERO_EXTEND_FUNC(16, 32);
ZERO_EXTEND_FUNC(32, 64);
SIGN_EXTEND_FUNC(8, 32);
SIGN_EXTEND_FUNC(16, 32);
SIGN_EXTEND_FUNC(32, 64);
