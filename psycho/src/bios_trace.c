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
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "psycho/cpu_defs.h"
#include "bios_trace.h"
#include "bus.h"
#include "cpu_defs.h"
#include "log.h"
#include "util.h"
#include "str.h"

LOG_MOD(P_LOG_BIOS);

enum {
	JR_RA = 0x03E00008,
};

static void on_putchar(struct p_ctx *ctx, const struct p_bios_frame *frame);

static const struct p_bios_fn a0_funcs[] = {
	// clang-format off

	[0x17]	= {
		.prototype	= "int strcmp(const char *s1=%s, const char *s2=%s)",
		.ret		= P_BIOS_FN_RET_INT
	},

	[0x25]	= {
		.prototype	= "char toupper(char c=%c)",
		.ret		= P_BIOS_FN_RET_CHAR
	},

	[0x2A]	= {
		.prototype	= "void *memcpy(unsigned char *s1=%p, const unsigned char *s2=%p, int n=%d)",
		.ret		= P_BIOS_FN_RET_VOID_PTR
	},

	[0x44]	= {
		.prototype	= "void FlushCache(void)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x96]	= {
		.prototype	= "void AddCDROMDevice(void)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x97]	= {
		.prototype	= "void AddMemCardDevice(void)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x99] = {
		.prototype	= "void add_nullcon_driver()",
		.ret		= P_BIOS_FN_RET_VOID
	}

	// clang-format on
};

static const struct p_bios_fn b0_funcs[] = {
	// clang-format off

	[0x00]	= {
		.prototype	= "void alloc_kernel_memory(size_t size=%d)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x18]	= {
		.prototype	= "void *ResetEntryInt(void)",
		.ret		= P_BIOS_FN_RET_VOID_PTR
	},

	[0x3D]	= {
		.prototype	= "void putchar(char c=%c)",
		.ret		= P_BIOS_FN_RET_VOID,
		.hook_cb	= on_putchar
	},

	[0x47]	= {
		.prototype	= "void AddDrv()",
		.ret		= P_BIOS_FN_RET_VOID
	}

	// clang-format on
};

static const struct p_bios_fn c0_funcs[] = {
	// clang-format off

	[0x00]	= {
		.prototype	= "void EnqueueTimerAndVblankIrqs(int prio=%d)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x01]	= {
		.prototype	= "void EnqueueSyscallHandler(int prio=%d)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x07]	= {
		.prototype	= "void InstallExceptionHandlers(void)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x08]	= {
		.prototype	= "void SysInitMemory(void *addr=%p, size_t size=%d)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x0C]	= {
		.prototype	= "void InitDefInt(int prio=%d)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x12]	= {
		.prototype	= "void InstallDevices(int ttyflag=%d)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	[0x1C]	= {
		.prototype	= "void AdjustA0Table(void)",
		.ret		= P_BIOS_FN_RET_VOID
	},

	// clang-format on
};

P_NODISCARD __attribute__((nonnull)) static const struct p_bios_fn *
func_get(const struct p_bios_fn *const arr, const size_t arr_elems,
	 const u32 func)
{
	if (func >= arr_elems)
		return NULL;

	return arr[func].prototype ? &arr[func] : NULL;
}

__attribute__((nonnull)) static void
frame_init(struct p_ctx *const ctx, struct p_bios_frame *const frame,
	   const struct p_bios_fn *const fn)
{
	frame->fn = fn;
	frame->arg_pos = 0;

	frame->a0 = ctx->cpu.gpr[P_A0];
	frame->a1 = ctx->cpu.gpr[P_A1];
	frame->a2 = ctx->cpu.gpr[P_A2];
	frame->a3 = ctx->cpu.gpr[P_A3];

	frame->sp = ctx->cpu.gpr[P_SP];
	frame->ra = ctx->cpu.gpr[P_RA];

	p_str_init_fixed(&frame->str, frame->str_buf, sizeof(frame->str_buf));
}

P_NODISCARD __attribute__((nonnull)) static struct p_bios_frame *
stack_emplace(struct p_ctx *const ctx)
{
	if (ctx->bios_trace.stack.top >= sizeof(ctx->bios_trace.stack.frames))
		return NULL;

	return &ctx->bios_trace.stack.frames[ctx->bios_trace.stack.top++];
}

P_NODISCARD __attribute__((nonnull)) static struct p_bios_frame *
stack_pop(struct p_ctx *const ctx)
{
	if (!ctx->bios_trace.stack.top)
		return NULL;

	return &ctx->bios_trace.stack.frames[--ctx->bios_trace.stack.top];
}

P_NODISCARD __attribute__((nonnull)) static u32
get_arg(struct p_ctx *const ctx, struct p_bios_frame *const frame)
{
	if (frame->arg_pos <= P_A3)
		return (&frame->a0)[frame->arg_pos++];

	return p_load_word(ctx, frame->sp + 16 + (frame->arg_pos++ - 4) * 4);
}

__attribute__((nonnull)) static void rst_tty_strs(struct p_ctx *const ctx)
{
	p_str_init_fixed(&ctx->bios_trace.tty_orig.str,
			 ctx->bios_trace.tty_orig.buf,
			 sizeof(ctx->bios_trace.tty_orig.buf));

	p_str_init_fixed(&ctx->bios_trace.tty_log.str,
			 ctx->bios_trace.tty_log.buf,
			 sizeof(ctx->bios_trace.tty_log.buf));
}

P_NODISCARD static const char *esc_seq(const char c)
{
	switch (c) {
	case '\n':
		return "\\n";
	case '\r':
		return "\\r";
	case '\t':
		return "\\t";
	case '\v':
		return "\\v";
	case '\f':
		return "\\f";
	case '\b':
		return "\\b";
	case '\a':
		return "\\a";
	case '\\':
		return "\\\\";
	case '\'':
		return "\\\'";
	case '\"':
		return "\\\"";
	case '\0':
		return "\\0";

	default:
		return NULL;
	}
}

__attribute__((nonnull)) static void
process_char(struct p_ctx *const ctx, struct p_bios_frame *const frame)
{
	const char c = get_arg(ctx, frame);

	const char *const seq = esc_seq(c);

	if (seq)
		p_str_append(&frame->str, NULL, "'%s'", seq);
	else
		p_str_append(&frame->str, NULL, "'%c'", c);
}

__attribute__((nonnull)) static void
process_int(struct p_ctx *const ctx, struct p_bios_frame *const frame)
{
	p_str_append(&frame->str, NULL, "%" PRIu32, get_arg(ctx, frame));
}

__attribute__((nonnull)) static void
process_str(struct p_ctx *const ctx, struct p_bios_frame *const frame)
{
	const u32 ptr = get_arg(ctx, frame);

	if (!ctx->cfg.bios_trace.deref_ptrs)
		goto end;

	u8 *const area = p_get_mem_area(ctx, vaddr_to_paddr(ptr));

	if (!area)
		goto end;

	p_str_append(&frame->str, NULL, "\"%s\"", area);
	return;

end:
	p_str_append(&frame->str, NULL, "0x%08X", ptr);
}

__attribute__((nonnull)) static void
process_ptr(struct p_ctx *const ctx, struct p_bios_frame *const frame)
{
	p_str_append(&frame->str, NULL, "0x%08X", get_arg(ctx, frame));
}

__attribute__((nonnull)) static void
on_putchar(struct p_ctx *const ctx, const struct p_bios_frame *const frame)
{
	const char c = frame->a0;
	const char *const seq = esc_seq(c);

	p_str_append(&ctx->bios_trace.tty_orig.str, NULL, "%c", c);

	if (!seq) {
		p_str_append(&ctx->bios_trace.tty_log.str, NULL, "%c", c);
		return;
	}

	p_str_append(&ctx->bios_trace.tty_log.str, NULL, "%s", seq);

	if (c == '\n') {
		LOG_INFO(ctx, "[stdout] %s", ctx->bios_trace.tty_log.str.ptr);

		if (ctx->cfg.bios_trace.stdout_line)
			ctx->cfg.bios_trace.stdout_line(
				ctx, &ctx->bios_trace.tty_log.str);

		rst_tty_strs(ctx);
	}
}

__attribute__((nonnull)) static void
process_prototype(struct p_ctx *const ctx, struct p_bios_frame *const frame)
{
	const char *src = frame->fn->prototype;

	while (*src) {
		if (*src != '%') {
			p_str_append(&frame->str, NULL, "%c", *src++);
			continue;
		}

		const char next = *(src + 1);

		switch (next) {
		case 'c':
			process_char(ctx, frame);
			break;

		case 'd':
			process_int(ctx, frame);
			break;

		case 'p':
			process_ptr(ctx, frame);
			break;

		case 's':
			process_str(ctx, frame);
			break;

		default:
			p_str_append(&frame->str, NULL, "%c", *src++);
			break;
		}
		src += (sizeof("%%") - 1);
	}
}

void p_bios_trace_init(struct p_ctx *const ctx)
{
	rst_tty_strs(ctx);
}

void p_bios_trace_begin(struct p_ctx *const ctx)
{
	const u32 fn_num = ctx->cpu.gpr[P_T1];

	const struct p_bios_fn *fn;

	if (ctx->cpu.pc == 0x000000A0)
		fn = func_get(a0_funcs, ARRAY_SIZE(a0_funcs), fn_num);
	else if (ctx->cpu.pc == 0x000000B0)
		fn = func_get(b0_funcs, ARRAY_SIZE(b0_funcs), fn_num);
	else if (ctx->cpu.pc == 0x000000C0)
		fn = func_get(c0_funcs, ARRAY_SIZE(c0_funcs), fn_num);
	else
		return;

	if (!fn) {
		LOG_WARN(ctx,
			 "Unimplemented BIOS call: 0x%02X:0x%08X; ignoring",
			 ctx->cpu.pc, fn_num);
		return;
	}

	struct p_bios_frame *const frame = stack_emplace(ctx);
	if (!frame) {
		LOG_WARN(ctx, "BIOS call stack is too deep for us to handle");
		return;
	}

	frame_init(ctx, frame, fn);

	if (frame->fn->hook_cb)
		frame->fn->hook_cb(ctx, frame);

	process_prototype(ctx, frame);
}

void p_bios_trace_end(struct p_ctx *const ctx)
{
	if (ctx->cpu.instr != JR_RA)
		return;

	struct p_bios_frame *const frame = stack_pop(ctx);
	if (!frame)
		return;

	const u32 v0 = ctx->cpu.gpr[P_V0];

	switch (frame->fn->ret) {
	case P_BIOS_FN_RET_CHAR:
		LOG_DBG(ctx, "bios call: %s -> '%c'", frame->str.ptr, (char)v0);
		return;

	case P_BIOS_FN_RET_VOID_PTR:
		LOG_DBG(ctx, "bios call: %s -> 0x%08X", frame->str.ptr, v0);
		return;

	case P_BIOS_FN_RET_INT:
		LOG_DBG(ctx, "bios call: %s -> %u", frame->str.ptr, v0);
		return;

	case P_BIOS_FN_RET_VOID:
		LOG_DBG(ctx, "bios call: %s", frame->str.ptr);
		return;

	default:
		__builtin_unreachable();
	}
}
