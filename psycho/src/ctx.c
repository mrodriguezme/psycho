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
#include <stddef.h>
#include <string.h>

#include "bios_trace.h"
#include "bus.h"
#include "cpu.h"
#include "cpu_defs.h"
#include "gpu.h"
#include "log.h"
#include "sched.h"
#include "util.h"

LOG_MOD(P_LOG_CTX);

// clang-format off

#define KERNEL_INIT_PC	(UINT32_C(0x80030000))

// clang-format on

struct exe_hdr {
	const char id[0x008 - 0x000];
	const u8 zero[0x010 - 0x008];
	const u32 pc;
	const u32 gp;
	const u32 dst_ram;
	const u32 file_size;
	const u32 data_sect_addr;
	const u32 data_sect_size;
	const u32 bss_sect_addr;
	const u32 bss_sect_size;
	const u32 sp_fp_base;
	const u32 sp_fp_offs;
	const u8 resv_bios_fn[0x4C - 0x38];
	const u8 ascii_marker;
	const u8 ascii_or_zerofilled[1971];
	const u8 code;
} __attribute__((packed));

static_assert_offset(struct exe_hdr, id, 0x000);
static_assert_offset(struct exe_hdr, zero, 0x008);
static_assert_offset(struct exe_hdr, pc, 0x010);
static_assert_offset(struct exe_hdr, gp, 0x014);
static_assert_offset(struct exe_hdr, dst_ram, 0x018);
static_assert_offset(struct exe_hdr, file_size, 0x1C);
static_assert_offset(struct exe_hdr, data_sect_addr, 0x20);
static_assert_offset(struct exe_hdr, data_sect_size, 0x24);
static_assert_offset(struct exe_hdr, bss_sect_addr, 0x28);
static_assert_offset(struct exe_hdr, bss_sect_size, 0x2C);
static_assert_offset(struct exe_hdr, sp_fp_base, 0x30);
static_assert_offset(struct exe_hdr, sp_fp_offs, 0x34);
static_assert_offset(struct exe_hdr, resv_bios_fn, 0x38);
static_assert_offset(struct exe_hdr, ascii_marker, 0x4C);
static_assert_offset(struct exe_hdr, code, 0x800);

static void exe_inject(struct p_ctx *const ctx)
{
	const struct exe_hdr *exe = (const struct exe_hdr *)ctx->exe.data;

	LOG_INFO(ctx, "injecting exe (dst=0x%08X, size=%u)", exe->dst_ram,
		 exe->file_size);

	p_cpu_pc_set(ctx, exe->pc);
	p_cpu_gpr_set(ctx, P_GP, exe->gp);

	if (exe->sp_fp_base) {
		const u32 val = exe->sp_fp_base + exe->sp_fp_offs;

		p_cpu_gpr_set(ctx, P_SP, val);
		p_cpu_gpr_set(ctx, P_FP, val);
	}

	const u32 paddr = vaddr_to_paddr(exe->dst_ram);
	memcpy(&ctx->bus.ram[paddr], &exe->code, exe->file_size);

	memset(&ctx->exe, 0, sizeof(ctx->exe));
}

struct p_ctx_cfg *p_cfg_get(struct p_ctx *const ctx)
{
	return &ctx->cfg;
}

void p_init(struct p_ctx *const ctx)
{
	p_bios_trace_init(ctx);
	p_bus_init(ctx);
	p_gpu_init(ctx);

	p_rst(ctx);

	LOG_INFO(ctx, "initialized");
}

void p_rst(struct p_ctx *const ctx)
{
	p_sched_rst(ctx);
	p_cpu_rst(ctx);
	p_gpu_rst(ctx);

	LOG_INFO(ctx, "reset");
}

void p_step(struct p_ctx *const ctx)
{
	if ((ctx->exe.data) && (ctx->cpu.pc == KERNEL_INIT_PC))
		exe_inject(ctx);

	p_cpu_run(ctx, 1);
}

P_NODISCARD enum p_ctx_ret p_run_exe(struct p_ctx *const ctx,
				     const u8 *const exe, const size_t exe_size)
{
	if (exe_size < sizeof(struct exe_hdr))
		return P_EXE_SIZE_INVALID;

	const struct exe_hdr *hdr = (const struct exe_hdr *)exe;

	if (memcmp(hdr->id, "PS-X EXE", sizeof("PS-X EXE") - 1) != 0)
		return P_EXE_ID_INVALID;

	if (hdr->file_size != (exe_size - offsetof(struct exe_hdr, code)))
		return P_EXE_FILE_SIZE_INVALID;

	ctx->exe.data = exe;
	ctx->exe.size = exe_size;

	p_rst(ctx);
	LOG_INFO(ctx, "will inject exe");

	return P_OK;
}

void p_run_until_ev(struct p_ctx *const ctx)
{
	bool inject = false;

	for (;;) {
		if ((inject) && ctx->cpu.pc == KERNEL_INIT_PC) {
			exe_inject(ctx);
			inject = false;
		}

		if (ctx->exe.data) {
			inject = true;
			p_cpu_run(ctx, 1);
		} else
			p_cpu_run(ctx, 100);

		if (p_sched_run(ctx))
			break;
	}
}
