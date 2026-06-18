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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "str.h"

void p_str_init_fixed(struct p_str *str, char *const ptr, const size_t cap)
{
	memset(str, 0, sizeof(*str));

	str->ptr = ptr;
	str->ptr[0] = '\0';

	str->cap = cap;
	str->len = 0;
}

void p_str_append(struct p_str *const str, bool *const truncated,
		  const char *const fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	p_str_vappend(str, truncated, fmt, args);
	va_end(args);
}

void p_str_vappend(struct p_str *const str, bool *const truncated,
		   const char *const fmt, va_list args)
{
	const size_t rem = str->cap - str->len;

	va_list args_copy;
	va_copy(args_copy, args);

	const int ret = vsnprintf(&str->ptr[str->len], rem, fmt, args_copy);

	va_end(args_copy);

	assert(ret >= 0);

	if (ret < 0)
		return;

	if ((size_t)ret >= rem) {
		str->len = str->cap - 1;

		if (truncated)
			*truncated = true;

		return;
	}

	str->len += (size_t)ret;

	if (truncated)
		*truncated = false;
}

void p_str_pad(struct p_str *const str, const char c, const size_t count,
	       bool *const truncated)
{
	if (str->len >= count) {
		if (truncated)
			*truncated = false;

		return;
	}

	const size_t needed = count - str->len;
	const size_t rem = str->cap - str->len - 1;

	const size_t written = (needed > rem) ? rem : needed;
	memset(&str->ptr[str->len], c, written);

	str->len += written;
	str->ptr[str->len] = '\0';

	if (truncated)
		*truncated = (written != needed);
}
