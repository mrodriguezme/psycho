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

enum ring_buffer_overflow_policy {
	// Discard the oldest entry, append the new one
	RING_BUFFER_OVERFLOW_POLICY_DROP_OLDEST,

	// Discard the incoming value
	RING_BUFFER_OVERFLOW_POLICY_DROP_NEWEST,

	// Overwrite the most recently pushed entry
	RING_BUFFER_OVERFLOW_POLICY_REPLACE_NEWEST
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

#define RING_BUFFER_IMPL(name, type, policy)                                                  \
	PSYCHO_ALWAYS_INLINE void name##_clear(struct name *rb)                               \
	{                                                                                     \
		memset(rb, 0, sizeof(*rb));                                                   \
	}                                                                                     \
                                                                                              \
	PSYCHO_ALWAYS_INLINE void name##_init(struct name *rb)                                \
	{                                                                                     \
		name##_clear(rb);                                                             \
	}                                                                                     \
                                                                                              \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE size_t name##_mask(struct name *rb, size_t val) \
	{                                                                                     \
		return val & (ARRAY_SIZE(rb->data) - 1);                                      \
	}                                                                                     \
                                                                                              \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE size_t name##_size(struct name *rb)             \
	{                                                                                     \
		return rb->write - rb->read;                                                  \
	}                                                                                     \
                                                                                              \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE bool name##_full(struct name *rb)               \
	{                                                                                     \
		return name##_size(rb) == ARRAY_SIZE(rb->data);                               \
	}                                                                                     \
                                                                                              \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE bool name##_empty(struct name *rb)              \
	{                                                                                     \
		return rb->read == rb->write;                                                 \
	}                                                                                     \
                                                                                              \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE type *name##_emplace(struct name *rb)           \
	{                                                                                     \
		if (likely(!name##_full(rb)))                                                 \
			return &rb->data[name##_mask(rb, rb->write++)];                       \
                                                                                              \
		switch (policy) {                                                             \
		case RING_BUFFER_OVERFLOW_POLICY_DROP_OLDEST:                                 \
			rb->read++;                                                           \
			return &rb->data[name##_mask(rb, rb->write++)];                       \
                                                                                              \
		case RING_BUFFER_OVERFLOW_POLICY_DROP_NEWEST:                                 \
			return NULL;                                                          \
                                                                                              \
		case RING_BUFFER_OVERFLOW_POLICY_REPLACE_NEWEST:                              \
			return &rb->data[name##_mask(rb, rb->write - 1)];                     \
                                                                                              \
		default:                                                                      \
			unreachable();                                                        \
		}                                                                             \
	}                                                                                     \
                                                                                              \
	PSYCHO_ALWAYS_INLINE void name##_push(struct name *rb, type val)                      \
	{                                                                                     \
		type *slot = name##_emplace(rb);                                              \
                                                                                              \
		if (likely(slot))                                                             \
			*slot = (val);                                                        \
	}                                                                                     \
                                                                                              \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE type *name##_pop(struct name *rb)               \
	{                                                                                     \
		if (unlikely(name##_empty(rb)))                                               \
			return NULL;                                                          \
                                                                                              \
		return &rb->data[name##_mask(rb, rb->read++)];                                \
	}                                                                                     \
                                                                                              \
	PSYCHO_NODISCARD PSYCHO_ALWAYS_INLINE type *name##_back(struct name *rb)              \
	{                                                                                     \
		if (unlikely(name##_empty(rb)))                                               \
			return NULL;                                                          \
                                                                                              \
		return &rb->data[name##_mask(rb, rb->write - 1)];                             \
	}

#pragma GCC diagnostic pop
