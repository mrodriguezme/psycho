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

#include <stddef.h>
#include "fixed_string.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define PSYCHO_LOG_LEVEL_LIST(X) \
	X(OFF, "off")            \
	X(INFO, "info")          \
	X(WARN, "warn")          \
	X(ERROR, "err")          \
	X(DEBUG, "debug")        \
	X(TRACE, "trace")

#define PSYCHO_LOG_MODULE_LIST(X)               \
	X(CTX, "ctx")                           \
	X(CPU, "cpu")                           \
	X(BUS, "bus")                           \
	X(BIOS_CALL_TRACER, "bios call tracer") \
	X(SCHED, "sched")                       \
	X(GPU, "gpu")                           \
	X(INTC, "intc")                         \
	X(SIO0, "sio0")                         \
	X(DIGITAL_PAD, "digital pad")           \
	X(DMAC, "dmac")

struct psycho_ctx;

enum psycho_log_level {
#define X(id, name) PSYCHO_LOG_LEVEL_##id,
	PSYCHO_LOG_LEVEL_LIST(X)
#undef X
	// clang-format off

	PSYCHO_LOG_LEVEL_COUNT

	// clang-format on
};

enum psycho_log_module {
#define X(id, name) PSYCHO_LOG_MODULE_##id,
	PSYCHO_LOG_MODULE_LIST(X)
#undef X
	// clang-format off

	PSYCHO_LOG_MODULE_COUNT

	// clang-format on
};

struct psycho_log_msg {
	struct psycho_fixed_string str;
	enum psycho_log_module module;
	enum psycho_log_level level;
};

struct psycho_log_cfg {
	void (*log_cb)(struct psycho_ctx *ctx, struct psycho_log_msg *msg);
	enum psycho_log_level modules[PSYCHO_LOG_MODULE_COUNT];
};

#ifdef __cplusplus
}
#endif // __cplusplus
