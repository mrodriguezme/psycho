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

#include <string.h>

enum lifo_overflow_policy {
	// Discard the incoming value
	LIFO_OVERFLOW_POLICY_DROP_NEWEST,

	// Overwrite the current top entry with the incoming value
	LIFO_OVERFLOW_POLICY_REPLACE_TOP
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

#define LIFO_IMPL(name, type, policy)                                               \
	PSYCHO_ALWAYS_INLINE void name##_clear(struct name *st)                     \
	{                                                                           \
		memset(st, 0, sizeof(*st));                                         \
	}                                                                           \
                                                                                    \
	PSYCHO_ALWAYS_INLINE void name##_init(struct name *st)                      \
	{                                                                           \
		name##_clear(st);                                                   \
	}                                                                           \
                                                                                    \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE size_t name##_size(struct name *st)   \
	{                                                                           \
		return st->size;                                                    \
	}                                                                           \
                                                                                    \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE bool name##_full(struct name *st)     \
	{                                                                           \
		return st->size == ARRAY_SIZE(st->data);                            \
	}                                                                           \
                                                                                    \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE bool name##_empty(struct name *st)    \
	{                                                                           \
		return st->size == 0;                                               \
	}                                                                           \
                                                                                    \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE type *name##_emplace(struct name *st) \
	{                                                                           \
		if (likely(!name##_full(st)))                                       \
			return &st->data[st->size++];                               \
                                                                                    \
		switch (policy) {                                                   \
		case LIFO_OVERFLOW_POLICY_DROP_NEWEST:                              \
			return NULL;                                                \
                                                                                    \
		case LIFO_OVERFLOW_POLICY_REPLACE_TOP:                              \
			return &st->data[st->size - 1];                             \
                                                                                    \
		default:                                                            \
			unreachable();                                              \
		}                                                                   \
	}                                                                           \
                                                                                    \
	PSYCHO_ALWAYS_INLINE void name##_push(struct name *st, type val)            \
	{                                                                           \
		type *slot = name##_emplace(st);                                    \
                                                                                    \
		if (likely(slot))                                                   \
			*slot = (val);                                              \
	}                                                                           \
                                                                                    \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE type *name##_pop(struct name *st)     \
	{                                                                           \
		if (unlikely(name##_empty(st)))                                     \
			return NULL;                                                \
                                                                                    \
		return &st->data[--st->size];                                       \
	}                                                                           \
                                                                                    \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE type *name##_top(struct name *st)     \
	{                                                                           \
		if (unlikely(name##_empty(st)))                                     \
			return NULL;                                                \
                                                                                    \
		return &st->data[st->size - 1];                                     \
	}

#pragma GCC diagnostic pop
