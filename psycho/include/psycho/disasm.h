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

#include "compiler.h"
#include "cpu-defs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct psycho_ctx;

enum {
	PSYCHO_DISASM_LEN_MAX = 512,
};

struct psycho_disasm_cfg {
	bool tracing;
};

struct psycho_disasm {
	struct psycho_disasm_cfg cfg;
};

PSYCHO_NODISCARD const char *
psycho_disasm_gpr_get(const enum psycho_cpu_gpr reg);

void psycho_disasm_instr(struct psycho_ctx *ctx, char *dst, size_t *len,
			 uint32_t pc);

#ifdef __cplusplus
}
#endif // __cplusplus
