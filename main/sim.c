/*
 * sim.c - Hardware abstraction layer for running in a simulator on Linux
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>

#include "SDL.h"

#include "hal.h"
#include "debug.h"
#include "timer.h"
#include "gfx.h"


static SDL_Window *win;
static SDL_Surface *surf;
static SDL_Renderer *rend;
static SDL_Texture *tex;
static bool quit = 0;


/* --- Logging ------------------------------------------------------------- */


void vdebug(const char *fmt, va_list ap)
{
	static struct timeval t0;
	static bool first = 1;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (first) {
		t0 = tv;
		first = 0;
	}
	tv.tv_sec -= t0.tv_sec;
	tv.tv_usec -= t0.tv_usec;
	if (tv.tv_usec < 0) {
		tv.tv_sec--;
		tv.tv_usec += 1000 * 1000;
	}
	printf("[%3u.%03u] ",
	    (unsigned) tv.tv_sec, (unsigned) tv.tv_usec / 1000);
	vprintf(fmt, ap);
}


/* --- Delays and sleeping ------------------------------------------------- */


void mdelay(unsigned ms)
{
	msleep(ms);
}


void msleep(unsigned ms)
{
	struct timespec req = {
		.tv_sec = ms / 1000,
		.tv_nsec = (ms % 1000) * 1000000,
	};

	nanosleep(&req, NULL);
}


/* --- Event loop ---------------------------------------------------------- */


static bool process_events(void)
{
	static bool touch_is_down = 0;
	SDL_Event event;

	if (!SDL_PollEvent(&event))
		return 0;

	switch (event.type) {
	case SDL_QUIT:
		quit = 1;
		return 1;
	case SDL_KEYDOWN:
		switch (event.key.keysym.sym) {
		case SDLK_q:
			quit = 1;
			return 1;
		case SDLK_SPACE:
			button_event(1);
			break;
		default:
			break;
		}
		break;
	case SDL_KEYUP:
		switch (event.key.keysym.sym) {
		case SDLK_SPACE:
			button_event(0);
			break;
		}
		break;
	case SDL_MOUSEMOTION:
		if (!touch_is_down)
			return 1;
debug("SDL_MOUSEMOTION\n");
		if (event.motion.state & SDL_BUTTON_LMASK)
			touch_move_event(event.motion.x, event.motion.y);
		else
			touch_up_event();
		break;
	case SDL_MOUSEBUTTONDOWN:
debug("SDL_MOUSEBUTTONDOWN\n");
		assert(!touch_is_down);
		if (event.button.state == SDL_BUTTON_LEFT)
			touch_down_event(event.motion.x, event.motion.y);
		touch_is_down = 1;
		break;
	case SDL_MOUSEBUTTONUP:
debug("SDL_MOUSEBUTTONUP\n");
		/*
		 * SDL sends mouse up and move events when hovering. Our touch
		 * screen (probably) can't do this, so we filter such events.
		 */
		if (!touch_is_down)
			return 1;
		touch_up_event();
		touch_is_down = 0;
		break;
	default:
		break;
	}
	return 1;
}


static void event_loop(void)
{
	unsigned uptime = 1;

	while (!quit) {
		if (!process_events()) {
			timer_tick(uptime);
			msleep(10);
			uptime += 10;
		}
	}
}


/* --- Display update ------------------------------------------------------ */


void update_display(struct gfx_drawable *da)
{
	const gfx_color *p;
	unsigned x, y;

	if (!da->changed)
		return;
	gfx_reset(da);

debug("update\n");
	assert(da->w == GFX_WIDTH);
	assert(da->h == GFX_HEIGHT);
	assert(da->damage.w <= GFX_WIDTH);
	assert(da->damage.h <= GFX_HEIGHT);
	for (y = da->damage.y; y != da->damage.y + da->damage.h; y++) {
		p = da->fb + y * da->w + da->damage.x;
		for (x = da->damage.x; x != da->damage.x + da->damage.w; x++) {
			SDL_Rect rect = {
				.x = x,
				.y = y,
				.w = 1,
				.h = 1 
			};

			SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format,
			    *p & 0xf8,
			    (*p & 7) << 5 | (*p & 0xe000) << 2,
			    (*p & 0x1f00) >> 5));
			p++;
		}
	}

	SDL_UpdateTexture(tex, NULL, surf->pixels, surf->pitch);
	SDL_RenderClear(rend);
	SDL_RenderCopy(rend, tex, NULL, NULL);
	SDL_RenderPresent(rend);
}


/* --- Display on/off ------------------------------------------------------ */


void display_on(bool on)
{
}


/* --- Initialization ------------------------------------------------------ */


static void init_sdl(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	surf = SDL_CreateRGBSurface(0, GFX_WIDTH, GFX_HEIGHT, 16,
	    0x1f << 11, 0x3f << 5, 0x1f, 0);
	if (!surf) {
		fprintf(stderr, "SDL_CreateRGBSurface: %s\n", SDL_GetError());
		exit(1);
	}

	win = SDL_CreateWindow("Anelok", SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED, surf->w, surf->h, 0);
	if (!win) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		exit(1);
	}

	rend = SDL_CreateRenderer(win, -1, 0);
	if (!rend) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		exit(1);
	}

	tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING, surf->w, surf->h);
	if (!tex) {
		fprintf(stderr, "SDL_CreateTexture: %s\n",
		    SDL_GetError());
		exit(1);
	}
}
	

/* --- Command-line processing --------------------------------------------- */


static void usage(const char *name)
{
	fprintf(stderr, "usage: %s [demo-number]\n", name);
	exit(1);
}


int main(int argc, char **argv)
{
	int param = 0;
	char *end;
	int c;

	while ((c = getopt(argc, argv, "")) != EOF)
		switch (c) {
		default:
			usage(*argv);
		}
	switch (argc - optind) {
	default:
		case 0:
			break;
		case 1:
			param = strtoul(argv[optind], &end, 0);
			if (*end)
				usage(*argv);
			break;
		usage(*argv);
	}

	init_sdl();
	app_init(param);
	event_loop();

	return 0;
}
