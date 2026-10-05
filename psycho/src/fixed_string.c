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

#include "fixed_string.h"

void psycho_fixed_string_init(struct psycho_fixed_string *str, char *ptr, size_t capacity)
{
	memset(str, 0, sizeof(*str));

	str->ptr    = ptr;
	str->ptr[0] = '\0';

	str->capacity = capacity;
	str->len      = 0;
}

void psycho_fixed_string_append(struct psycho_fixed_string *str, bool *truncated, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	psycho_fixed_string_vappend(str, truncated, fmt, args);
	va_end(args);
}

void psycho_fixed_string_vappend(struct psycho_fixed_string *str, bool *truncated, const char *fmt, va_list args)
{
	size_t remaining = str->capacity - str->len;

	va_list args_copy;
	va_copy(args_copy, args);

	const int ret = vsnprintf(&str->ptr[str->len], remaining, fmt, args_copy);

	va_end(args_copy);

	assert(ret >= 0);

	if (ret < 0)
		return;

	if ((size_t)ret >= remaining) {
		str->len = str->capacity - 1;

		if (truncated)
			*truncated = true;

		return;
	}

	str->len += (size_t)ret;

	if (truncated)
		*truncated = false;
}

void psycho_fixed_string_pad(struct psycho_fixed_string *str, char c, size_t count, bool *truncated)
{
	if (str->len >= count) {
		if (truncated)
			*truncated = false;

		return;
	}

	size_t needed	 = count - str->len;
	size_t remaining = str->capacity - str->len - 1;

	size_t written = (needed > remaining) ? remaining : needed;
	memset(&str->ptr[str->len], c, written);

	str->len += written;
	str->ptr[str->len] = '\0';

	if (truncated)
		*truncated = (written != needed);
}
