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
#include "common/util.h"

PSYCHO_ALWAYS_INLINE PSYCHO_NODISCARD size_t node_parent(size_t node)
{
	return (node - 1) / 2;
}

PSYCHO_ALWAYS_INLINE PSYCHO_NODISCARD size_t node_left(size_t node)
{
	return (node * 2) + 1;
}

PSYCHO_ALWAYS_INLINE PSYCHO_NODISCARD size_t node_right(size_t node)
{
	return node_left(node) + 1;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

#define BINARY_HEAP_IMPL(name, type, cmp_func)                                                                        \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE void name##_reset(struct name *heap)                                      \
	{                                                                                                             \
		memset(heap, 0, sizeof(struct name));                                                                 \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE void name##_init(struct name *heap)                                       \
	{                                                                                                             \
		name##_reset(heap);                                                                                   \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE type name##_root(struct name *heap)                                       \
	{                                                                                                             \
		return heap->data[0];                                                                                 \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE bool name##_full(struct name *heap)                                       \
	{                                                                                                             \
		return heap->size == ARRAY_SIZE(heap->data);                                                          \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE void name##_sift_up(struct name *heap, size_t node)                       \
	{                                                                                                             \
		while (node) {                                                                                        \
			size_t parent_node = node_parent(node);                                                       \
                                                                                                                      \
			if (!cmp_func(heap->data[node], heap->data[parent_node]))                                     \
				break;                                                                                \
                                                                                                                      \
			swap(&heap->data[parent_node], &heap->data[node]);                                            \
			node = parent_node;                                                                           \
		}                                                                                                     \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE void name##_sift_down(struct name *heap, size_t node)                     \
	{                                                                                                             \
		for (;;) {                                                                                            \
			size_t smallest_node = node;                                                                  \
			size_t left_node     = node_left(node);                                                       \
			size_t right_node    = node_right(node);                                                      \
                                                                                                                      \
			if ((left_node < heap->size) && cmp_func(heap->data[left_node], heap->data[smallest_node]))   \
				smallest_node = left_node;                                                            \
                                                                                                                      \
			if ((right_node < heap->size) && cmp_func(heap->data[right_node], heap->data[smallest_node])) \
				smallest_node = right_node;                                                           \
                                                                                                                      \
			if (smallest_node == node)                                                                    \
				break;                                                                                \
                                                                                                                      \
			swap(&heap->data[node], &heap->data[smallest_node]);                                          \
			node = smallest_node;                                                                         \
		}                                                                                                     \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE void name##_resift(struct name *heap, size_t node)                        \
	{                                                                                                             \
		name##_sift_up(heap, node);                                                                           \
		name##_sift_down(heap, node);                                                                         \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE void name##_insert(struct name *heap, type data)                          \
	{                                                                                                             \
		if (unlikely(name##_full(heap)))                                                                      \
			return;                                                                                       \
                                                                                                                      \
		size_t node	 = heap->size++;                                                                      \
		heap->data[node] = data;                                                                              \
                                                                                                                      \
		name##_sift_up(heap, node);                                                                           \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE void name##_delete_at(struct name *heap, size_t node)                     \
	{                                                                                                             \
		if (node >= heap->size)                                                                               \
			return;                                                                                       \
                                                                                                                      \
		heap->size--;                                                                                         \
                                                                                                                      \
		if (node < heap->size) {                                                                              \
			heap->data[node] = heap->data[heap->size];                                                    \
			name##_resift(heap, node);                                                                    \
		}                                                                                                     \
	}                                                                                                             \
                                                                                                                      \
	PSYCHO_NONNULL PSYCHO_ALWAYS_INLINE void name##_delete(struct name *heap, type data)                          \
	{                                                                                                             \
		for (size_t node = 0; node < heap->size; ++node) {                                                    \
			if (heap->data[node] == data) {                                                               \
				name##_delete_at(heap, node);                                                         \
				return;                                                                               \
			}                                                                                             \
		}                                                                                                     \
	}

#pragma GCC diagnostic pop
