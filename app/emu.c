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

#include "emu.h"
#include "ansi-color-codes.h"

static struct emu_runner *m_emu;
u16 btns_prev = 0;

static void emu_poll_ctrl(struct emu_runner *emu)
{
	const bool *keys	      = SDL_GetKeyboardState(NULL);
	enum p_digital_ctrl_btns mask = 0;

	if (keys[SDL_SCANCODE_DOWN])
		mask |= P_DIGITAL_CTRL_DN;

	if (keys[SDL_SCANCODE_UP])
		mask |= P_DIGITAL_CTRL_UP;

	if (keys[SDL_SCANCODE_LEFT])
		mask |= P_DIGITAL_CTRL_LT;

	if (keys[SDL_SCANCODE_RIGHT])
		mask |= P_DIGITAL_CTRL_RT;

	if (keys[SDL_SCANCODE_X])
		mask |= P_DIGITAL_CTRL_CROSS;

	if (keys[SDL_SCANCODE_O])
		mask |= P_DIGITAL_CTRL_CIR;

	if (keys[SDL_SCANCODE_S])
		mask |= P_DIGITAL_CTRL_SQR;

	if (keys[SDL_SCANCODE_T])
		mask |= P_DIGITAL_CTRL_TRI;

	if (keys[SDL_SCANCODE_RETURN])
		mask |= P_DIGITAL_CTRL_START;

	if (keys[SDL_SCANCODE_SPACE])
		mask |= P_DIGITAL_CTRL_SEL;

	enum p_digital_ctrl_btns pressed  = mask & ~btns_prev;
	enum p_digital_ctrl_btns released = btns_prev & ~mask;

	if (pressed)
		p_digital_ctrl_btn_press(&emu->ctrl, pressed);

	if (released)
		p_digital_ctrl_btn_rel(&emu->ctrl, released);

	btns_prev = mask;
}

static void on_vblank(struct p_ctx *ctx)
{
	int idx = SDL_GetAtomicInt(&m_emu->write_idx);
	memcpy(m_emu->fbufs[idx], ctx->gpu.vram, sizeof(m_emu->fbufs[idx]));
	SDL_SetAtomicInt(&m_emu->write_idx, idx ^ 1);

	SDL_AddAtomicInt(&m_emu->frame_count, 1);
}

static void illegal_instr_cb(struct p_ctx *ctx, u32 instr)
{
	(void)instr;

	assert(ctx != NULL);

	fflush(stdout);
	abort();
}

static void log_cb(struct p_ctx *ctx, struct p_log_msg *msg)
{
	assert(ctx != NULL);
	assert(msg != NULL);

	static const char *color_str[P_LOG_COUNT] = {
		[P_LOG_INFO]  = BHWHT "%s\n" CRESET,
		[P_LOG_WARN]  = BHYEL "%s\n" CRESET,
		[P_LOG_ERR]   = BHRED "%s\n" CRESET,
		[P_LOG_DBG]   = BHCYN "%s\n" CRESET,
		[P_LOG_TRACE] = BHMAG "%s\n" CRESET
	};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
	printf(color_str[msg->lvl], msg->str.ptr);
#pragma GCC diagnostic pop
}

static void on_stdout_line(struct p_ctx *ctx, struct p_str *str)
{
	(void)ctx;
	(void)str;
}

void emu_init(struct emu_runner *emu, u8 *bios_data, u8 *exe_data,
	      size_t exe_size)
{
	m_emu = emu;

	struct p_ctx_cfg *const cfg = p_cfg_get(&emu->ctx);

	cfg->cpu.illegal_instr = illegal_instr_cb;

	cfg->log.log_cb = log_cb;

	cfg->log.mod[P_LOG_CTX]		 = P_LOG_TRACE;
	cfg->log.mod[P_LOG_BIOS]	 = P_LOG_INFO;
	cfg->log.mod[P_LOG_DIGITAL_CTRL] = P_LOG_TRACE;
	cfg->log.mod[P_LOG_SIO0]	 = P_LOG_TRACE;
	cfg->log.mod[P_LOG_SCHED]	 = P_LOG_TRACE;

	cfg->bios_trace.stdout_line = on_stdout_line;
	cfg->bios_trace.deref_ptrs  = true;

	cfg->on_vblank = on_vblank;

	// I'm lazy.
	u8 *data = p_bios_data_get(&emu->ctx);
	memcpy(data, bios_data, P_BUS_BIOS_SIZE_BYTES);

	p_init(&emu->ctx);

	p_digital_ctrl_init(&emu->ctx, &emu->ctrl);
	p_attach_dev_to_sio0(&emu->ctx, &emu->ctrl.dev, 0);

	if (!p_run_exe(&emu->ctx, exe_data, exe_size)) {
		fprintf(stderr, "exe not valid\n");
		exit(EXIT_FAILURE);
	}
}

static int emu_thread_func(void *data)
{
	u32 last_frame_count = SDL_GetAtomicInt(&m_emu->frame_count);

	while (SDL_GetAtomicInt(&m_emu->running)) {
		emu_poll_ctrl(m_emu);

		const Uint64 start_ns = SDL_GetTicksNS();
		p_run_until_ev(&m_emu->ctx);

		u32 current_frame_count = SDL_GetAtomicInt(&m_emu->frame_count);

		if (current_frame_count == last_frame_count)
			continue;

		const Uint64 end_ns = SDL_GetTicksNS();
		const Uint64 diff   = end_ns - start_ns;
		const Uint64 target = 1000000000 / 60;

#if 0
		if (diff < target)
			SDL_DelayNS(target - diff);
#endif
		last_frame_count = current_frame_count;
	}
	return 0;
}

void emu_run(struct emu_runner *emu)
{
	emu->thread = SDL_CreateThread(emu_thread_func, "Emulator", emu);

	if (!emu->thread) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "SDL_CreateThread() failed: %s", SDL_GetError());
		abort();
	}
}

void emu_stop(struct emu_runner *emu)
{
	SDL_SetAtomicInt(&emu->running, 0);
	SDL_WaitThread(emu->thread, NULL);
}
