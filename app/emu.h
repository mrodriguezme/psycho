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

#include <SDL3/SDL.h>

#include "psycho/ctx.h"
#include "psycho/digital_ctrl.h"

struct emu_runner {
	struct p_ctx ctx;
	struct p_digital_ctrl ctrl;

	p_gpu_vram fbufs[2];
	SDL_AtomicInt write_idx;
	SDL_AtomicInt running;
	SDL_AtomicInt frame_pend;
	SDL_AtomicInt frame_count;
	SDL_Thread *thread;
};

void emu_init(struct emu_runner *emu, u8 *bios_data, u8 *exe_data,
	      size_t exe_size);

void emu_run(struct emu_runner *emu);

void emu_stop(struct emu_runner *emu);

P_NONNULL P_ALWAYS_INLINE u16 *emu_front_buffer_get(struct emu_runner *emu)
{
	return (u16 *)emu->fbufs[SDL_GetAtomicInt(&emu->write_idx) ^ 1];
}

void emu_btn_press(struct emu_runner *emu, SDL_Event *ev);
void emu_btn_rel(struct emu_runner *emu, SDL_Event *ev);
