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

#include <string.h>

#include "exe_loader.h"
#include "log.h"
#include "util.h"
#include "cpu_defs.h"

LOG_MOD(P_LOG_CTX);

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

P_NODISCARD enum p_ctx_ret p_run_exe(struct p_ctx *ctx, u8 *exe, size_t size)
{
	if (unlikely(size < sizeof(struct exe_hdr)))
		return P_EXE_SIZE_INVALID;

	const struct exe_hdr *hdr = (const struct exe_hdr *)exe;

	if (unlikely(memcmp(hdr->id, "PS-X EXE", sizeof("PS-X EXE") - 1) != 0))
		return P_EXE_ID_INVALID;

	if (unlikely(hdr->file_size != (size - offsetof(struct exe_hdr, code))))
		return P_EXE_FILE_SIZE_INVALID;

	ctx->exe.data = exe;
	ctx->exe.size = size;

	p_rst(ctx);
	LOG_INFO(ctx, "will inject exe");

	return P_OK;
}

void p_exe_inject(struct p_ctx *ctx)
{
	const struct exe_hdr *exe = (const struct exe_hdr *)ctx->exe.data;

	LOG_INFO(ctx, "injecting exe (dst=0x%08X, size=%u)", exe->dst_ram,
		 exe->file_size);

	ctx->cpu.pc_set(ctx, exe->pc);
	ctx->cpu.gpr_set(ctx, P_GP, exe->gp);

	if (exe->sp_fp_base) {
		const u32 val = exe->sp_fp_base + exe->sp_fp_offs;

		ctx->cpu.gpr_set(ctx, P_SP, val);
		ctx->cpu.gpr_set(ctx, P_FP, val);
	}

	const u32 paddr = vaddr_to_paddr(exe->dst_ram);
	memcpy(&ctx->bus.ram[paddr], &exe->code, exe->file_size);

	memset(&ctx->exe, 0, sizeof(ctx->exe));
}
