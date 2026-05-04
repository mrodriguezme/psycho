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
#include <ctype.h>
#include <stdio.h>

#include "psycho/cpu-defs.h"
#include "bus.h"
#include "bios-trace.h"
#include "cpu-defs.h"
#include "log.h"
#include "types.h"
#include "util.h"

LOG_MODULE(PSYCHO_LOG_MODULE_BIOS);

enum {
	JR_RA = 0x03E00008,
};

static const struct psycho_bios_func a0_funcs[] = {
	// clang-format off

	[0x17]	= {
		.prototype	= "int strcmp(const char *s1=%s, const char *s2=%s)",
		.ret		= PSYCHO_BIOS_FUNC_RET_INT
	},

	[0x25]	= {
		.prototype	= "char toupper(char c)",
		.ret		= PSYCHO_BIOS_FUNC_RET_CHAR
	},

	[0x2A]	= {
		.prototype	= "void *memcpy(unsigned char *s1=%p, const unsigned char *s2=%p, int n=%d)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID_PTR
	},

	[0x44]	= {
		.prototype	= "void FlushCache(void)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x96]	= {
		.prototype	= "void AddCDROMDevice(void)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x97]	= {
		.prototype	= "void AddMemCardDevice(void)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x99] = {
		.prototype	= "void add_nullcon_driver()",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	}

	// clang-format on
};

static const struct psycho_bios_func b0_funcs[] = {
	// clang-format off

	[0x00]	= {
		.prototype	= "void alloc_kernel_memory(size_t size=%d)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x18]	= {
		.prototype	= "void *ResetEntryInt(void)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID_PTR
	},

	[0x3D]	= {
		.prototype	= "void putchar(char c=%c)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x47]	= {
		.prototype	= "void AddDrv()",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	}

	// clang-format on
};

static const struct psycho_bios_func c0_funcs[] = {
	// clang-format off

	[0x00]	= {
		.prototype	= "void EnqueueTimerAndVblankIrqs(int prio=%d)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x01]	= {
		.prototype	= "void EnqueueSyscallHandler(int prio=%d)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x07]	= {
		.prototype	= "void InstallExceptionHandlers(void)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x08]	= {
		.prototype	= "void SysInitMemory(void *addr=%p, size_t size=%d)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x0C]	= {
		.prototype	= "void InitDefInt(int prio=%d)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x12]	= {
		.prototype	= "void InstallDevices(int ttyflag=%d)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	[0x1C]	= {
		.prototype	= "void AdjustA0Table(void)",
		.ret		= PSYCHO_BIOS_FUNC_RET_VOID
	},

	// clang-format on
};

PSYCHO_NODISCARD static const struct psycho_bios_func *
func_get(const struct psycho_bios_func *const arr, const size_t arr_elems,
	 const u32 func)
{
	if (func >= arr_elems)
		return NULL;

	return arr[func].prototype ? &arr[func] : NULL;
}

PSYCHO_NODISCARD static struct psycho_bios_frame *
stack_emplace(struct psycho_ctx *const ctx)
{
	if (ctx->bios_trace.stack.top >= PSYCHO_BIOS_TRACE_STACK_MAX)
		return NULL;

	return &ctx->bios_trace.stack.frames[ctx->bios_trace.stack.top++];
}

PSYCHO_NODISCARD static struct psycho_bios_frame *
stack_pop(struct psycho_ctx *const ctx)
{
	if (!ctx->bios_trace.stack.top)
		return NULL;

	return &ctx->bios_trace.stack.frames[--ctx->bios_trace.stack.top];
}

PSYCHO_NODISCARD static u32 get_arg(struct psycho_ctx *const ctx,
				    const struct psycho_bios_frame *const frame,
				    const u32 idx)
{
	if (idx <= PSYCHO_CPU_REG_A3)
		return (&frame->a0)[idx];

	return psycho_bus_load_word(ctx, frame->sp + 16 + (idx - 4) * 4);
}

PSYCHO_NODISCARD static const char *escape_seq(const char c)
{
	switch (c) {
	case '\n':
		return "\\n";

	default:
		return NULL;
	}
}

static size_t process_char(char *dst, char c)
{
	unsigned char uc = (unsigned char)c;

	const char *esc = escape_seq(uc);

	if (esc)
		return (size_t)sprintf(dst, "'%s'", esc);

	if (isprint(uc))
		return (size_t)sprintf(dst, "'%c'", uc);

	return (size_t)sprintf(dst, "'\\x%02X'", uc);
}

static size_t process_str(struct psycho_ctx *const ctx, char *const dst,
			  u32 ptr)
{
	if (!ctx->bios_trace.cfg.deref_ptrs)
		goto end;

	ptr = cpu_vaddr_to_paddr(ptr);
	u8 *area = psycho_bus_get_mem_area(ctx, ptr);

	if (!area)
		goto end;

	return sprintf(dst, "\"%s\"", area);

end:
	return sprintf(dst, "0x%08X", ptr);
}

static void process_prototype(struct psycho_ctx *const ctx,
			      struct psycho_bios_frame *const frame)
{
	assert(ctx != NULL);

#define SPECIFIER_LEN (2)

	const char *src = frame->func->prototype;
	char *dst = frame->result;

	u32 idx = 0;

	while (*src) {
		if (*src != '%') {
			*dst++ = *src++;
			continue;
		}

		const char next = *(src + 1);

		switch (next) {
		case 'c':
			dst += process_char(dst, get_arg(ctx, frame, idx++));
			break;

		case 'd':
			dst += sprintf(dst, "%u", get_arg(ctx, frame, idx++));
			break;

		case 'p':
			dst += sprintf(dst, "0x%08X",
				       get_arg(ctx, frame, idx++));
			break;

		case 's':
			dst += process_str(ctx, dst,
					   get_arg(ctx, frame, idx++));
			break;

		default:
			*dst++ = *src++;
			break;
		}
		src += SPECIFIER_LEN;
	}

	*dst = '\0';

#undef SPECIFIER_LEN
}

void psycho_bios_trace_init(struct psycho_ctx *const ctx,
			    const struct psycho_bios_trace_cfg *const cfg)
{
	ctx->bios_trace.cfg = *cfg;
	LOG_INFO(ctx, "initialized");
}

void psycho_bios_trace_begin(struct psycho_ctx *const ctx)
{
	const u32 func = ctx->cpu.gpr[PSYCHO_CPU_REG_T1];

	const struct psycho_bios_func *func_data;

	if (ctx->cpu.pc == 0x000000A0)
		func_data = func_get(a0_funcs, ARRAY_SIZE(a0_funcs), func);
	else if (ctx->cpu.pc == 0x000000B0)
		func_data = func_get(b0_funcs, ARRAY_SIZE(b0_funcs), func);
	else if (ctx->cpu.pc == 0x000000C0)
		func_data = func_get(c0_funcs, ARRAY_SIZE(c0_funcs), func);
	else
		return;

	if (func_data == NULL) {
		LOG_TRACE(ctx,
			  "Unimplemented BIOS call: 0x%02X:0x%08X; ignoring",
			  ctx->cpu.pc, func);
		return;
	}

	struct psycho_bios_frame *frame = stack_emplace(ctx);
	if (!frame)
		return;

	frame->func = func_data;
	frame->a0 = ctx->cpu.gpr[PSYCHO_CPU_REG_A0];
	frame->a1 = ctx->cpu.gpr[PSYCHO_CPU_REG_A1];
	frame->a2 = ctx->cpu.gpr[PSYCHO_CPU_REG_A2];
	frame->a3 = ctx->cpu.gpr[PSYCHO_CPU_REG_A3];
	frame->sp = ctx->cpu.gpr[PSYCHO_CPU_REG_SP];
	frame->ra = ctx->cpu.gpr[PSYCHO_CPU_REG_RA];

	process_prototype(ctx, frame);
}

void psycho_bios_trace_end(struct psycho_ctx *const ctx)
{
	assert(ctx != NULL);

	if (ctx->cpu.instr != JR_RA)
		return;

	struct psycho_bios_frame *frame = stack_pop(ctx);
	if (!frame)
		return;

	const u32 v0 = ctx->cpu.gpr[PSYCHO_CPU_REG_V0];

	const char *fmt;
	bool ret = true;

	switch (frame->func->ret) {
	case PSYCHO_BIOS_FUNC_RET_CHAR:
		fmt = "bios call: %s -> '%c'";
		break;

	case PSYCHO_BIOS_FUNC_RET_VOID_PTR:
		fmt = "bios call: %s -> 0x%08X";
		break;

	case PSYCHO_BIOS_FUNC_RET_INT:
		fmt = "bios call: %s -> %d";
		break;

	case PSYCHO_BIOS_FUNC_RET_VOID:
	default:
		fmt = "bios call: %s";
		ret = false;

		break;
	}

	if (ret)
		LOG_DBG(ctx, fmt, frame->result, v0);
	else
		LOG_DBG(ctx, fmt, frame->result);
}
