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

void emu_btn_press(struct emu_runner *emu, SDL_Event *ev)
{
	if (ev->key.repeat)
		return;

	switch (ev->key.key) {
	case SDLK_DOWN:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_DN);
		break;

	case SDLK_UP:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_UP);
		break;

	case SDLK_LEFT:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_LT);
		break;

	case SDLK_RIGHT:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_RT);
		break;

	case SDLK_X:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_CROSS);
		break;

	case SDLK_O:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_CIR);
		break;

	case SDLK_S:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_SQR);
		break;

	case SDLK_T:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_TRI);
		break;

	case SDLK_RETURN:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_START);
		break;

	case SDLK_SPACE:
		p_digital_ctrl_btn_press(&emu->ctrl, P_DIGITAL_CTRL_SEL);
		break;

	default:
		break;
	}
}

void emu_btn_rel(struct emu_runner *emu, SDL_Event *ev)
{
	switch (ev->key.key) {
	case SDLK_DOWN:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_DN);
		break;

	case SDLK_UP:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_UP);
		break;

	case SDLK_LEFT:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_LT);
		break;

	case SDLK_RIGHT:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_RT);
		break;

	case SDLK_X:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_CROSS);
		break;

	case SDLK_O:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_CIR);
		break;

	case SDLK_S:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_SQR);
		break;

	case SDLK_T:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_TRI);
		break;

	case SDLK_RETURN:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_START);
		break;

	case SDLK_SPACE:
		p_digital_ctrl_btn_rel(&emu->ctrl, P_DIGITAL_CTRL_SEL);
		break;

	default:
		break;
	}
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

	cfg->log.mod[P_LOG_CTX]	  = P_LOG_TRACE;
	cfg->log.mod[P_LOG_BIOS]  = P_LOG_INFO;
	//cfg->log.mod[P_LOG_SIO0] = P_LOG_TRACE;

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
		const Uint64 start_ns = SDL_GetTicksNS();
		p_run_until_ev(&m_emu->ctx);

		u32 current_frame_count = SDL_GetAtomicInt(&m_emu->frame_count);

		if (current_frame_count == last_frame_count)
			continue;

		const Uint64 end_ns = SDL_GetTicksNS();
		const Uint64 diff   = end_ns - start_ns;
		const Uint64 target = 1000000000 / 60;

		if (diff < target)
			;
			//SDL_DelayNS(target - diff);

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
