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

static struct psycho_ctx m_ctx;
static const char *prog_name;

static void log_cb(struct psycho_ctx *const ctx,
		   const struct psycho_log_msg_data *const msg)
{
	assert(ctx != NULL);
	assert(msg != NULL);

	static const char *const color_str[PSYCHO_LOG_LEVEL_COUNT] = {
		// clang-format off

		[PSYCHO_LOG_LEVEL_INFO]		= BHWHT "%s\n" CRESET,
		[PSYCHO_LOG_LEVEL_WARN]		= BHYEL "%s\n" CRESET,
		[PSYCHO_LOG_LEVEL_ERR]		= BHRED "%s\n" CRESET,
		[PSYCHO_LOG_LEVEL_DBG]		= BHCYN "%s\n" CRESET,
		[PSYCHO_LOG_LEVEL_TRACE]	= BHMAG "%s\n" CRESET

		// clang-format on
	};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
	printf(color_str[msg->level], msg->msg);
#pragma GCC diagnostic pop
}

static void illegal_instr_cb(struct psycho_ctx *const ctx, const uint32_t instr)
{
	assert(ctx != NULL);

	fflush(stdout);
	abort();
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

	if (file_size != PSYCHO_BUS_BIOS_SIZE) {
		fprintf(stderr,
			"%s: bios file size is not correct (expected %d bytes, got %zu)\n",
			prog_name, PSYCHO_BUS_BIOS_SIZE, file_size);
		return false;
	}

	FILE *fd = fopen(bios_file, "rb");

	if (!fd) {
		fprintf(stderr, "%s: unable to open bios file %s: %s\n",
			prog_name, bios_file, strerror(errno));
		return false;
	}

	uint8_t *dst = psycho_bus_bios_data_get(&m_ctx);
	const size_t bytes_read = fread(dst, sizeof(uint8_t), file_size, fd);

	if ((bytes_read != file_size) || (ferror(fd))) {
		fprintf(stderr,
			"%s: not all bytes were read from bios file %s: %s\n",
			prog_name, bios_file, strerror(errno));
		fclose(fd);

		return false;
	}
	fclose(fd);
	return true;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "%s: missing required argument\n", argv[0]);
		fprintf(stderr, "%s: syntax: %s <bios_file>\n", argv[0],
			argv[0]);

		return EXIT_FAILURE;
	}

	prog_name = argv[0];

	const struct psycho_ctx_cfg cfg = {
		// clang-format off

		.cpu	= {
			.illegal_instr	= illegal_instr_cb
		},

		.log	= {
			.log_cb		= log_cb,
			.modules	= {
				[PSYCHO_LOG_MODULE_CTX]	= PSYCHO_LOG_LEVEL_TRACE,
				[PSYCHO_LOG_MODULE_CPU]	= PSYCHO_LOG_LEVEL_TRACE,
				[PSYCHO_LOG_MODULE_BUS]	= PSYCHO_LOG_LEVEL_TRACE,
			}
		}

		// clang-format on
	};

	psycho_ctx_init(&m_ctx, &cfg);

	if (!load_bios_file(argv[1]))
		return EXIT_FAILURE;

	for (;;)
		psycho_ctx_step(&m_ctx);

	return EXIT_SUCCESS;
}
