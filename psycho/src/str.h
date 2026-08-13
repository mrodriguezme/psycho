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

#include <stdarg.h>
#include <stdbool.h>

#include "psycho/compiler.h"
#include "psycho/str.h"

void p_str_init_fixed(struct p_str *str, char *ptr, size_t cap) P_NONNULL;

void p_str_rst(struct p_str *str) P_NONNULL;

void p_str_append(struct p_str *str, bool *truncated, const char *fmt, ...)
	__attribute__((format(printf, 3, 4), nonnull(1, 3)));

void p_str_vappend(struct p_str *str, bool *truncated, const char *fmt,
		   va_list args)
	__attribute__((format(printf, 3, 0), nonnull(1, 3)));

void p_str_pad(struct p_str *str, char c, size_t count, bool *truncated)
	__attribute__((nonnull(1)));
