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

#include "psycho/ctx.h"

void p_bus_init(struct p_ctx *ctx) P_NONNULL;

P_NODISCARD u32 p_load32(struct p_ctx *ctx, u32 paddr) P_NONNULL;

P_NODISCARD u16 p_load16(struct p_ctx *ctx, u32 paddr) P_NONNULL;

P_NODISCARD u8 p_load8(struct p_ctx *ctx, const u32 paddr) P_NONNULL;

void p_store32(struct p_ctx *ctx, u32 paddr, u32 word) P_NONNULL;

void p_store16(struct p_ctx *ctx, u32 paddr, u16 halfword) P_NONNULL;

void p_store8(struct p_ctx *ctx, u32 paddr, u8 byte) P_NONNULL;

P_PURE void *p_get_mem_area(struct p_ctx *ctx, u32 paddr) P_NONNULL;
