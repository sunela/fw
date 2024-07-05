/*
 * sim.c - Hardware abstraction layer for running in a simulator on Linux
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#include "SDL.h"

#include "hal.h"
#include "debug.h"
#include "timer.h"
#include "gfx.h"
#include "ui.h"
#include "storage.h"
#include "script.h"
#include "sim.h"


#define	DEFAULT_SCREENSHOT_NAME	"screen%04u.ppm"


bool headless = 0;
const char *screenshot_name = DEFAULT_SCREENSHOT_NAME;
unsigned screenshot_number = 0;


static SDL_Window *win;
static SDL_Surface *surf;
static SDL_Renderer *rend;
static SDL_Texture *tex;
static unsigned zoom = 1;
static bool quit = 0;


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


/* --- Display update ------------------------------------------------------ */


static void render(void)
{
	SDL_RenderClear(rend);
	SDL_RenderCopy(rend, tex, NULL, NULL);
	SDL_RenderPresent(rend);
}


#define	CORNER_R	40


static void hline(unsigned x0, unsigned x1, unsigned y)
{
	SDL_Rect rect = {
		.x = x0 * zoom,
		.y = y * zoom,
		.w = (x1 - x0 + 1) * zoom,
		.h = zoom
	};

	SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format, 30, 30, 30));
}


static void cut_corners(void)
{
	unsigned x, y;

	for (y = 0; y <= CORNER_R; y++) {
		x = sqrt(CORNER_R * CORNER_R - y * y);
		hline(0, CORNER_R - x - 1, CORNER_R - y);
		hline(GFX_WIDTH - CORNER_R + x, GFX_WIDTH - 1, CORNER_R - y);
		hline(0, CORNER_R - x - 1, GFX_HEIGHT - 1 - CORNER_R + y);
		hline(GFX_WIDTH - CORNER_R + x, GFX_WIDTH - 1,
		    GFX_HEIGHT - 1 - CORNER_R + y);
	}
}


static void pixel(unsigned x, unsigned y, gfx_color c)
{
	SDL_Rect rect = {
		.x = x * zoom,
		.y = y * zoom,
		.w = zoom,
		.h = zoom
	};

	SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format,
	    c & 0xf8,
	    (c & 7) << 5 | (c & 0xe000) << 2,
	    (c & 0x1f00) >> 5));
}


void update_display_partial(struct gfx_drawable *da, unsigned x, unsigned y)
{
	const gfx_color *p = da->fb;
	unsigned h, i;

debug("update_display_partial(w %u, h %u, x %u, y %u)\n", da->w, da->h, x, y);
	for (h = da->h; h; h--) {
		for (i = 0; i != da->w; i++)
			pixel(x + i, y, *p++);
		y++;
	}
	cut_corners();

	SDL_UpdateTexture(tex, NULL, surf->pixels, surf->pitch);
	render();
}


void update_display(struct gfx_drawable *da)
{
	const gfx_color *p;
	int x, y;

	if (headless)
		return;
	if (!da->changed)
		return;
	gfx_reset(da);

//debug("update\n");
	assert(da->w == GFX_WIDTH);
	assert(da->h == GFX_HEIGHT);
	assert(da->damage.x >= 0);
	assert(da->damage.y >= 0);
	assert(da->damage.w <= GFX_WIDTH);
	assert(da->damage.h <= GFX_HEIGHT);
	for (y = da->damage.y; y != da->damage.y + da->damage.h; y++) {
		p = da->fb + y * da->w + da->damage.x;
		for (x = da->damage.x; x != da->damage.x + da->damage.w; x++)
			pixel(x, y, *p++);
	}
	cut_corners();

	SDL_UpdateTexture(tex, NULL, surf->pixels, surf->pitch);
	render();
}


/* --- Event loop ---------------------------------------------------------- */


static bool process_events(void)
{
	static bool touch_is_down = 0;
	static int last_x = 0;
	static int last_y = 0;
	SDL_Event event;

	if (!SDL_PollEvent(&event))
		return 0;

	switch (event.type) {
	case SDL_QUIT:
		quit = 1;
		return 1;
	case SDL_KEYDOWN:
		switch (event.key.keysym.sym) {
		case SDLK_c:
			printf("X %d Y %d\n", last_x, last_y);
			break;
		case SDLK_s:
			if (!screenshot(&main_da, screenshot_name,
			    screenshot_number))
				return 1;
			fprintf(stderr, "screenshut %u\n", screenshot_number);
			screenshot_number++;
			break;
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
		last_x = event.motion.x / zoom;
		last_y = event.motion.y / zoom;
		if (!touch_is_down)
			return 1;
//debug("SDL_MOUSEMOTION\n");
		if (event.motion.state & SDL_BUTTON_LMASK)
			touch_move_event(last_x, last_y);
		else
			touch_up_event();
		break;
	case SDL_MOUSEBUTTONDOWN:
//debug("SDL_MOUSEBUTTONDOWN\n");
		assert(!touch_is_down);
		last_x = event.button.x / zoom;
		last_y = event.button.y / zoom;
		if (event.button.state == SDL_BUTTON_LEFT)
			touch_down_event(last_x, last_y);
		touch_is_down = 1;
		break;
	case SDL_MOUSEBUTTONUP:
//debug("SDL_MOUSEBUTTONUP\n");
		last_x = event.button.x / zoom;
		last_y = event.button.y / zoom;
		/*
		 * SDL sends mouse up and move events when hovering. Our touch
		 * screen (probably) can't do this, so we filter such events.
		 */
		if (!touch_is_down)
			return 1;
		touch_up_event();
		touch_is_down = 0;
		break;
	case SDL_WINDOWEVENT:
		render();
		break;
	default:
//debug("event %u (0x%x)\n", event.type, event.type);
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
			tick_event();
			msleep(10);
			uptime += 10;
		}
	}
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

	surf = SDL_CreateRGBSurface(0, GFX_WIDTH * zoom, GFX_HEIGHT * zoom, 16,
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
	fprintf(stderr,
"usage: %s [options]\n"
"%6s %s [options] demo-name [demo-arg ...]] [-C command ...]\n"
"\n"
"-2  double the pixel size\n"
"-C  receive user interaction from a script\n"
"-d database\n"
"    set the database file (default: %s)\n"
"-q  quiet. Disable debugging output.\n"
"-s screenshot\n"
"    set the screenshot file name. if present, %%u is converted to the\n"
"    screenshot number (starts at 0). The usual printf conversion\n"
"    specifications can be used. (default: %s)\n"
    , name, "", name, DEFAULT_DB_FILE_NAME, DEFAULT_SCREENSHOT_NAME);
	exit(1);
}


int main(int argc, char **argv)
{
	bool scripting = 0;
	int c, i;

	while ((c = getopt(argc, argv, "+2Cd:qs:")) != EOF)
		switch (c) {
		case '2':
			zoom = 2;
			break;
		case 'C':
			scripting = 1;
			break;
		case 'd':
			storage_file = optarg;
			break;
		case 'q':
			quiet = 1;
			break;
		case 's':
			screenshot_name = optarg;
			break;
		default:
			usage(*argv);
		}

	for (i = optind; i != argc; i++)
		if (!strcmp(argv[i], "-C"))
			break;
	db_init();
	if (i == argc && !scripting) {
		init_sdl();
		if (!app_init(argv + optind, argc - optind))
			usage(*argv);
	} else {
		headless = 1;
		if (scripting) {
			app_init(NULL, 0);
			if (!run_script(argv + optind, argc - optind))
				return 1;
		} else {
			if (!app_init(argv + optind, i - optind)) 
				usage(*argv);
			if (!run_script(argv + i + 1, argc - i - 1))
				return 1;
		}
		if (headless)
			return 0;
		init_sdl();
		update_display(&main_da);
	}
	event_loop();

	return 0;
}
