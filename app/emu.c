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
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "ansi_color_codes.h"
#include "emu.h"

#include "psycho/ctx.h"
#include "psycho/devices/digital_pad.h"

static uint8_t *exe_file_data;
static size_t exe_file_size;
static uint16_t buttons_prev;

static void load_exe_file(const char *exe_file)
{
	if (!(exe_file_data = SDL_LoadFile(exe_file, &exe_file_size))) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load EXE: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
}

static void load_bios_file(struct emu_runner *emu, const char *bios_file)
{
	SDL_IOStream *file = SDL_IOFromFile(bios_file, "rb");

	if (!file) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Loading BIOS file failed: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	Sint64 size = SDL_GetIOSize(file);

	if (size < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Getting BIOS size failed: %s", SDL_GetError());
		goto cleanup;
	}

	if (size != PSYCHO_BUS_BIOS_SIZE_BYTES) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "BIOS file size is wrong (expected exactly %d, got %ld)",
			     PSYCHO_BUS_BIOS_SIZE_BYTES, size);

		goto cleanup;
	}

	uint8_t *dst = p_bios_data_get(&emu->ctx);

	size_t bytes_read = SDL_ReadIO(file, dst, size);

	if (bytes_read != (size_t)size) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Not all bytes were read from BIOS file (expected exactly %d, got %zu)",
			     PSYCHO_BUS_BIOS_SIZE_BYTES, bytes_read);

		goto cleanup;
	}

	SDL_CloseIO(file);
	return;

cleanup:
	SDL_CloseIO(file);
	exit(EXIT_FAILURE);
}

static void poll_pad(struct emu_runner *emu)
{
	const bool *keys = SDL_GetKeyboardState(NULL);

	enum psycho_digital_pad_button mask = 0;

	if (keys[SDL_SCANCODE_DOWN])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_DOWN;

	if (keys[SDL_SCANCODE_UP])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_UP;

	if (keys[SDL_SCANCODE_LEFT])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_LEFT;

	if (keys[SDL_SCANCODE_RIGHT])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_RIGHT;

	if (keys[SDL_SCANCODE_X])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_CROSS;

	if (keys[SDL_SCANCODE_O])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_CIRCLE;

	if (keys[SDL_SCANCODE_S])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_SQUARE;

	if (keys[SDL_SCANCODE_T])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_TRIANGLE;

	if (keys[SDL_SCANCODE_RETURN])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_START;

	if (keys[SDL_SCANCODE_SPACE])
		mask |= PSYCHO_DIGITAL_PAD_BUTTON_SELECT;

	enum psycho_digital_pad_button pressed	= mask & ~buttons_prev;
	enum psycho_digital_pad_button released = buttons_prev & ~mask;

	if (pressed)
		psycho_digital_pad_buttons_press(&emu->pad, pressed);

	if (released)
		psycho_digital_pad_buttons_release(&emu->pad, released);

	buttons_prev = mask;
}

static void on_vblank(struct p_ctx *ctx, void *userdata)
{
	struct emu_runner *emu = userdata;

	poll_pad(emu);

	int idx = SDL_GetAtomicInt(&emu->write_idx);
	memcpy(emu->fbufs[idx], ctx->gpu.vram, sizeof(emu->fbufs[idx]));

	SDL_SetAtomicInt(&emu->write_idx, idx ^ 1);
	SDL_AddAtomicInt(&emu->frame_count, 1);

#if 0
	Uint64 frame_ns = UINT64_C(1000000000) / 60;
	Uint64 now      = SDL_GetTicksNS();

	if (!emu->next_frame_ns)
		emu->next_frame_ns = now;

	emu->next_frame_ns += frame_ns;

	if (now < emu->next_frame_ns)
		SDL_DelayPrecise(emu->next_frame_ns - now);
	else if ((now - emu->next_frame_ns) > (frame_ns * 4))
		emu->next_frame_ns = now;
#endif

	if (!SDL_GetAtomicInt(&emu->running))
		psycho_ctx_stop(ctx);
}

static void illegal_instr_cb(struct p_ctx *ctx, uint32_t instr)
{
	(void)instr;

	assert(ctx != NULL);

	fflush(stdout);
	abort();
}

static void log_cb(struct p_ctx *ctx, struct psycho_log_msg *msg)
{
	assert(ctx != NULL);
	assert(msg != NULL);

	static const char *color_str[PSYCHO_LOG_LEVEL_COUNT] = {
		// clang-format off

		[PSYCHO_LOG_LEVEL_INFO]		= BHWHT "%s\n" CRESET,
		[PSYCHO_LOG_LEVEL_WARN]		= BHYEL "%s\n" CRESET,
		[PSYCHO_LOG_LEVEL_ERROR]	= BHRED "%s\n" CRESET,
		[PSYCHO_LOG_LEVEL_DEBUG]	= BHCYN "%s\n" CRESET,
		[PSYCHO_LOG_LEVEL_TRACE]	= BHMAG "%s\n" CRESET

		// clang-format on
	};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
	printf(color_str[msg->level], msg->str.ptr);
#pragma GCC diagnostic pop
}

static void on_stdout_line(struct p_ctx *ctx, struct p_str *str)
{
	(void)ctx;
	(void)str;
}

void emu_init(struct emu_runner *emu, const char *bios_file, const char *exe_file)
{
	load_bios_file(emu, bios_file);
	load_exe_file(exe_file);

	struct psycho_ctx_cfg *cfg = p_cfg_get(&emu->ctx);

	cfg->cpu.illegal_instr = illegal_instr_cb;

	cfg->log.log_cb = log_cb;

	cfg->log.modules[PSYCHO_LOG_MODULE_BUS] = PSYCHO_LOG_LEVEL_TRACE;
	//cfg->log.modules[PSYCHO_LOG_MODULE_SCHED] = PSYCHO_LOG_LEVEL_TRACE;

	//cfg->log.modules[PSYCHO_LOG_MODULE_BIOS_CALL_TRACER] = PSYCHO_LOG_LEVEL_TRACE;

	cfg->bios_call_tracer.stdout_line = on_stdout_line;
	cfg->bios_call_tracer.deref_ptrs  = true;

	cfg->on_vblank	     = on_vblank;
	cfg->vblank_userdata = emu;

	p_init(&emu->ctx);

	psycho_digital_pad_init(&emu->ctx, &emu->pad);
	psycho_sio0_device_attach(&emu->pad.dev, PSYCHO_SIO0_SLOT_1);

	if (!p_run_exe(&emu->ctx, exe_file_data, exe_file_size)) {
		fprintf(stderr, "exe not valid\n");
		exit(EXIT_FAILURE);
	}
}

static int run_loop(void *data)
{
	struct emu_runner *emu = data;

	SDL_SetAtomicInt(&emu->running, 1);
	psycho_ctx_run(&emu->ctx);

	return 0;
}

void emu_run(struct emu_runner *emu)
{
	emu->thread = SDL_CreateThread(run_loop, "Emulator", emu);

	if (!emu->thread) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateThread() failed: %s", SDL_GetError());
		abort();
	}
}

void emu_stop(struct emu_runner *emu)
{
	SDL_SetAtomicInt(&emu->running, 0);
	SDL_WaitThread(emu->thread, NULL);
}
