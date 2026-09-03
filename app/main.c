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
#include <sys/stat.h>
#include <SDL3/SDL.h>
#include <errno.h>
#include <stdlib.h>

#include "emu.h"

static const char *prog_name;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

static struct emu_runner emu;
static bool vsync_enabled;

static u8 *exe_data;
static size_t exe_size;
static u8 bios_data[P_BUS_BIOS_SIZE_BYTES];

static void update_window_title(int render_fps, int emu_fps)
{
	char title[128];
	SDL_snprintf(title, sizeof(title),
		     "psycho | Host FPS: %d | Emulator FPS: %d", render_fps,
		     emu_fps);
	SDL_SetWindowTitle(window, title);
}

static bool get_file_size(const char *file, size_t *file_size)
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

static bool load_exe_file(char *exe_file)
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

static bool load_bios_file(char *bios_file)
{
	assert(bios_file != NULL);

	size_t file_size;

	if (!get_file_size(bios_file, &file_size))
		return false;

	if (file_size != P_BUS_BIOS_SIZE_BYTES) {
		fprintf(stderr,
			"%s: bios file size is not correct (expected %d bytes, "
			"got %zu)\n",
			prog_name, P_BUS_BIOS_SIZE_BYTES, file_size);
		return false;
	}

	FILE *const handle = fopen(bios_file, "rb");

	if (!handle) {
		fprintf(stderr, "%s: unable to open bios file %s: %s\n",
			prog_name, bios_file, strerror(errno));
		return false;
	}

	const size_t bytes =
		fread(bios_data, sizeof(uint8_t), file_size, handle);

	if (bytes != file_size) {
		fprintf(stderr,
			"%s: not all bytes were read from bios file %s: %s\n",
			prog_name, bios_file, strerror(errno));
		fclose(handle);

		return false;
	}
	fclose(handle);
	return true;
}

static bool gfx_init(void)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "SDL_Init() failed: %s", SDL_GetError());
		return false;
	}

	if (!SDL_CreateWindowAndRenderer("psycho", 1920, 1080,
					 SDL_WINDOW_RESIZABLE, &window,
					 &renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "SDL_CreateWindowAndRenderer() failed: %s",
			     SDL_GetError());
		return false;
	}

	vsync_enabled = SDL_SetRenderVSync(renderer, 1);
	if (!vsync_enabled) {
		SDL_Log("vsync not supported, falling back to manual frame "
			"cap: %s",
			SDL_GetError());
	} else
		SDL_Log("vsync enabled");

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR1555,
				    SDL_TEXTUREACCESS_STREAMING, VRAM_WIDTH,
				    VRAM_HEIGHT);

	if (!texture) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "SDL_CreateTexture() failed: %s", SDL_GetError());
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
	u16 *src = emu_front_buffer_get(&emu);

	void *pixels;
	int pitch;

	if (!SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
		SDL_Log("SDL_LockTexture failed: %s", SDL_GetError());
		return;
	}

	const size_t row_bytes = VRAM_WIDTH * sizeof(u16);

	if (pitch == (int)row_bytes) {
		memcpy(pixels, src, row_bytes * VRAM_HEIGHT);
	} else {
		u8 *src_bytes = (u8 *)src;
		u8 *dst_bytes = (u8 *)pixels;
		for (int y = 0; y < VRAM_HEIGHT; ++y)
			memcpy(dst_bytes + y * pitch, src_bytes + y * row_bytes,
			       row_bytes);
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

	case SDL_EVENT_KEY_DOWN:
		emu_btn_press(&emu, ev);
		break;

	case SDL_EVENT_KEY_UP:
		emu_btn_rel(&emu, ev);
		break;

	default:
		break;
	}
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

	if (!load_bios_file(argv[1]))
		return EXIT_FAILURE;

	if (!load_exe_file(argv[2]))
		return EXIT_FAILURE;

	emu_init(&emu, bios_data, exe_data, exe_size);

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
			int current_emu_frame_count =
				SDL_GetAtomicInt(&emu.frame_count);
			int emu_fps =
				current_emu_frame_count - last_emu_frame_count;

			update_window_title(render_frame_count, emu_fps);

			render_frame_count   = 0;
			last_emu_frame_count = current_emu_frame_count;
			fps_window_start     = now;
		}

		const Uint64 frame_end = SDL_GetTicksNS();

		if (!vsync_enabled) {
			const Uint64 diff      = frame_end - frame_start;
			const Uint64 target    = 1000000000 / 60;

			if (diff < target)
				SDL_DelayNS(target - diff);
		}
	}

	emu_stop(&emu);

	gfx_fini();
	return EXIT_SUCCESS;
}
