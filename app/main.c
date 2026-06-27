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
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ansi-color-codes.h"
#include "psycho/ctx.h"

static struct p_ctx m_ctx;
static u8 *exe_data;
static size_t exe_size;
static const char *prog_name;

static void log_cb(struct p_ctx *const ctx, const struct p_log_msg *const msg)
{
	assert(ctx != NULL);
	assert(msg != NULL);

	static const char *const color_str[P_LOG_COUNT] = {
		// clang-format off

		[P_LOG_INFO]	= BHWHT "%s\n" CRESET,
		[P_LOG_WARN]	= BHYEL "%s\n" CRESET,
		[P_LOG_ERR]	= BHRED "%s\n" CRESET,
		[P_LOG_DBG]	= BHCYN "%s\n" CRESET,
		[P_LOG_TRACE]	= BHMAG "%s\n" CRESET

		// clang-format on
	};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
	printf(color_str[msg->lvl], msg->str.ptr);
#pragma GCC diagnostic pop
}

static void illegal_instr_cb(struct p_ctx *const ctx, const uint32_t instr)
{
	assert(ctx != NULL);

	fflush(stdout);
	abort();
}

static void on_stdout_line(struct p_ctx *const ctx, struct p_str *const str)
{
}

static bool get_file_size(const char *const file, size_t *const file_size)
{
	struct stat st;

	if (stat(file, &st) < 0) {
		fprintf(stderr, "%s: unable to get file size of %s: %s",
			prog_name, file, strerror(errno));

		return false;
	}

	*file_size = st.st_size;
	return true;
}

static bool load_bios_file(const char *const bios_file)
{
	assert(bios_file != NULL);

	size_t file_size;

	if (!get_file_size(bios_file, &file_size))
		return false;

	if (file_size != P_BUS_BIOS_SIZE) {
		fprintf(stderr,
			"%s: bios file size is not correct (expected %d bytes, got %zu)\n",
			prog_name, P_BUS_BIOS_SIZE, file_size);
		return false;
	}

	FILE *handle = fopen(bios_file, "rb");

	if (!handle) {
		fprintf(stderr, "%s: unable to open bios file %s: %s\n",
			prog_name, bios_file, strerror(errno));
		return false;
	}

	uint8_t *dst = p_bios_data_get(&m_ctx);
	const size_t bytes_read =
		fread(dst, sizeof(uint8_t), file_size, handle);

	if (bytes_read != file_size) {
		fprintf(stderr,
			"%s: not all bytes were read from bios file %s: %s\n",
			prog_name, bios_file, strerror(errno));
		fclose(handle);

		return false;
	}
	fclose(handle);
	return true;
}

static bool load_exe_file(const char *const exe_file)
{
	assert(exe_file != NULL);

	size_t file_size;

	if (!get_file_size(exe_file, &file_size))
		return false;

	FILE *handle = fopen(exe_file, "rb");

	if (!handle) {
		fprintf(stderr, "%s: unable to open exe file %s: %s\n",
			prog_name, exe_file, strerror(errno));
		return false;
	}

	exe_data = malloc(file_size);
	exe_size = fread(exe_data, sizeof(uint8_t), file_size, handle);

	if (exe_size != file_size) {
		fprintf(stderr,
			"%s: not all bytes were read from exe file %s: %s\n",
			prog_name, exe_file, strerror(errno));

		fclose(handle);
		free(exe_data);

		return false;
	}
	fclose(handle);
	return true;
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "%s: missing required argument\n", argv[0]);
		fprintf(stderr, "%s: syntax: %s <bios_file> <exe_file>\n",
			argv[0], argv[0]);

		return EXIT_FAILURE;
	}

	prog_name = argv[0];

	struct p_ctx_cfg *const cfg = p_cfg_get(&m_ctx);

	cfg->cpu.illegal_instr = illegal_instr_cb;

	cfg->log.log_cb = log_cb;
	cfg->log.mod[P_LOG_CTX] = P_LOG_TRACE;
	cfg->log.mod[P_LOG_CPU] = P_LOG_TRACE;
	cfg->log.mod[P_LOG_BUS] = P_LOG_TRACE;
	cfg->log.mod[P_LOG_BIOS] = P_LOG_TRACE;
	cfg->log.mod[P_LOG_SCHED] = P_LOG_TRACE;

	cfg->bios_trace.stdout_line = on_stdout_line;
	cfg->bios_trace.deref_ptrs = true;

	cfg->disasm.tracing = true;

	p_init(&m_ctx);

	if (!load_bios_file(argv[1]))
		return EXIT_FAILURE;

	if (!load_exe_file(argv[2]))
		return EXIT_FAILURE;

	if (!p_run_exe(&m_ctx, exe_data, exe_size)) {
		fprintf(stderr, "%s: exe not valid\n", argv[2]);
		return EXIT_FAILURE;
	}

	for (;;)
		p_run_until_ev(&m_ctx);

	return EXIT_SUCCESS;
}
