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

#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdlib.h>

#include "emu.h"

static const char *prog_name;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static bool vsync_enabled;

static struct emu_runner emu;

static void update_window_title(int render_fps, int emu_fps)
{
	char title[128];
	SDL_snprintf(title, sizeof(title), "psycho | Host FPS: %d | Emulator FPS: %d", render_fps, emu_fps);
	SDL_SetWindowTitle(window, title);
}

static bool gfx_init(void)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init() failed: %s", SDL_GetError());
		return false;
	}

	if (!SDL_CreateWindowAndRenderer("psycho", 1920, 1080, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindowAndRenderer() failed: %s", SDL_GetError());
		return false;
	}

	vsync_enabled = SDL_SetRenderVSync(renderer, 1);

	if (!vsync_enabled)
		SDL_Log("vsync not supported, falling back to manual frame cap: %s", SDL_GetError());
	else
		SDL_Log("vsync enabled");

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR1555, SDL_TEXTUREACCESS_STREAMING, VRAM_WIDTH,
				    VRAM_HEIGHT);

	if (!texture) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTexture() failed: %s", SDL_GetError());
		return false;
	}

	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

	return true;
}

static void gfx_fini(void)
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

static void render_frame(void)
{
	uint16_t *src = emu_front_buffer_get(&emu);

	void *pixels;
	int pitch;

	if (!SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
		SDL_Log("SDL_LockTexture failed: %s", SDL_GetError());
		return;
	}

	size_t row_bytes = VRAM_WIDTH * sizeof(uint16_t);

	if (pitch == (int)row_bytes)
		memcpy(pixels, src, row_bytes * VRAM_HEIGHT);
	else {
		uint8_t *src_bytes = (uint8_t *)src;
		uint8_t *dst_bytes = (uint8_t *)pixels;

		for (int y = 0; y < VRAM_HEIGHT; ++y)
			memcpy(dst_bytes + y * pitch, src_bytes + y * row_bytes, row_bytes);
	}

	SDL_UnlockTexture(texture);

	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

static void sdl_process_ev(SDL_Event *ev)
{
	switch (ev->type) {
	case SDL_EVENT_QUIT:
		SDL_SetAtomicInt(&emu.running, 0);
		break;

	default:
		break;
	}
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "%s: missing required argument\n", argv[0]);
		fprintf(stderr, "%s: syntax: %s <bios_file> <exe_file>\n", argv[0], argv[0]);

		return EXIT_FAILURE;
	}

	emu_init(&emu, argv[1], argv[2]);

	if (!gfx_init())
		return EXIT_FAILURE;

	SDL_SetAtomicInt(&emu.running, 1);
	emu_run(&emu);

	Uint64 fps_window_start	 = SDL_GetTicksNS();
	int last_emu_frame_count = SDL_GetAtomicInt(&emu.frame_count);
	int render_frame_count	 = 0;

	while (SDL_GetAtomicInt(&emu.running)) {
		const Uint64 frame_start = SDL_GetTicksNS();

		SDL_Event ev;
		while (SDL_PollEvent(&ev))
			sdl_process_ev(&ev);

		render_frame();
		render_frame_count++;

		Uint64 now = SDL_GetTicksNS();
		if (now - fps_window_start >= 1000000000ULL) {
			int current_emu_frame_count = SDL_GetAtomicInt(&emu.frame_count);
			int emu_fps		    = current_emu_frame_count - last_emu_frame_count;

			update_window_title(render_frame_count, emu_fps);

			render_frame_count   = 0;
			last_emu_frame_count = current_emu_frame_count;
			fps_window_start     = now;
		}

		const Uint64 frame_end = SDL_GetTicksNS();

		if (!vsync_enabled) {
			const Uint64 diff   = frame_end - frame_start;
			const Uint64 target = 1000000000 / 60;

			if (diff < target)
				SDL_DelayNS(target - diff);
		}
	}

	emu_stop(&emu);
	gfx_fini();

	return EXIT_SUCCESS;
}
