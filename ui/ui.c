/*
 * ui.c - User interface: setup and event distribution
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "hal.h"
#include "timer.h"
#include "gfx.h"
#include "long_text.h"
#include "ntext.h"
#include "pin.h"
#include "ui.h"

#ifndef SIM
#include "st7789.h"
#endif /* !SIM */


#define	CROSSHAIR	0

#define	UI_TIMERS	3


struct gfx_drawable da;

unsigned pin_cooldown = 0;
unsigned pin_attempts = 0;

static gfx_color fb[GFX_WIDTH * GFX_HEIGHT];
static const struct ui *current_ui = NULL;
static struct timer idle_timer;


/* --- Crosshair ----------------------------------------------------------- */


#if CROSSHAIR
static bool crosshair_shown = 0;
static unsigned crosshair_x = 0;
static unsigned crosshair_y = 0;
static gfx_color backing_fb_x[GFX_WIDTH];
static gfx_color backing_fb_y[GFX_HEIGHT];
static struct gfx_drawable backing_da_x;
static struct gfx_drawable backing_da_y;
#endif /* CROSSHAIR */


static void crosshair_remove(void)
{
#if CROSSHAIR
	if (crosshair_shown) {
		gfx_copy(&da, 0, crosshair_y, &backing_da_x, 0, 0,
		    GFX_WIDTH, 1, -1);
		gfx_copy(&da, crosshair_x, 0, &backing_da_y, 0, 0,
		    1, GFX_HEIGHT, -1);
		update_display(&da);
		crosshair_shown = 0;
	}
#endif /* CROSSHAIR */
}


static void crosshair_show(unsigned x, unsigned y)
{
#if CROSSHAIR
	if (crosshair_shown)
		crosshair_remove();
	gfx_da_init(&backing_da_x, GFX_WIDTH, 1, backing_fb_x);
	gfx_da_init(&backing_da_y, 1, GFX_HEIGHT, backing_fb_y);
	gfx_copy(&backing_da_x, 0, 0, &da, 0, y, GFX_WIDTH, 1, -1);
	gfx_copy(&backing_da_y, 0, 0, &da, x, 0, 1, GFX_HEIGHT -1, -1);
	crosshair_x = x;
	crosshair_y = y;
	gfx_rect_xy(&da, x, 0, 1, GFX_HEIGHT - 1, GFX_RED);
	gfx_rect_xy(&da, 0, y, GFX_WIDTH - 1, 1, GFX_RED);
	update_display(&da);
	crosshair_shown = 1;
#endif /* CROSSHAIR */
}


/* --- Show citrine logo --------------------------------------------------- */


void show_citrine(void)
{
#if 0
	static const uint16_t img[] = {
		#include "citrine.inc"
	};
	struct gfx_drawable da_img;
	unsigned w, h;

	w = img[0];
	h = img[1];

	gfx_da_init(&da_img, w, h, (uint16_t *) img + 2);
	gfx_copy(&da, (GFX_WIDTH - w) / 2, (GFX_HEIGHT - h) / 2,
	    &da_img, 0, 0, w, h, -1);
#endif
}


/* --- On/off button ------------------------------------------------------- */


static bool is_on = 0;


static void idle_off(void *user)
{
	turn_off();
}


void progress(void)
{
	timer_set(&idle_timer, 1000 * IDLE_S, idle_off, NULL);
}


static void turn_on(void)
{
	if (is_on)
		return;
	is_on = 1;
	// @@@ hal_...
	pin_shuffle_pad();
	progress();
	if (now < pin_cooldown)
		ui_switch(&ui_cooldown);
	else
		ui_switch(&ui_pin);
}


void turn_off(void)
{
	if (!is_on)
		return;
	is_on = 0;
	// @@@ hal_...
	ui_switch(&ui_off);
}


static void toggle_on_off(void)
{
debug("toggle_on_off %u\n", is_on);
	if (is_on)
		turn_off();
	else
		turn_on();
}


void button_event(bool down)
{
	static unsigned debounce  = 0;

	debug("button %u\n", down);
	if (now < debounce)
		return;
	if (!down) {
		debounce = 0;
		return;
	}
	if (debounce) {
		debounce = 0;
	} else {
		toggle_on_off();
		debounce = now + DEBOUNCE_MS;
	}
}


/* --- Touch screen / mouse ------------------------------------------------ */


//static unsigned touch_down_start = 0;


void touch_down_event(unsigned x, unsigned y)
{
	debug("mouse down %u %u\n", x, y);
	if (x >= GFX_WIDTH || y >= GFX_HEIGHT)
		return;
	if (current_ui && current_ui->events && current_ui->events->touch_tap)
		current_ui->events->touch_tap(x, y);
	crosshair_show(x, y);
}


void touch_move_event(unsigned x, unsigned y)
{
//	debug("mouse move %u %u\n", x, y);
	if (x >= GFX_WIDTH || y >= GFX_HEIGHT)
		return;
	crosshair_show(x, y);
}


void touch_up_event(void)
{
	debug("mouse up\n");
	crosshair_remove();
}


/* --- UI page selection --------------------------------------------------- */


void ui_switch(const struct ui *ui)
{
debug("ui_switch\n");
	if (current_ui) {
		if (current_ui->close)
			current_ui->close();
	}
	gfx_clear(&da, GFX_BLACK);
	current_ui = ui;
	if (ui->open)
		ui->open();
	update_display(&da);
}


/* --- Graphics demos/tests ------------------------------------------------ */


/* Simple rectangles and colors. */

static void demo_1(void)
{
	/* red, green, blue rectangle */
	gfx_rect_xy(&da, 50, 50, 150, 50, gfx_hex(0xff0000));
	gfx_rect_xy(&da, 50, 130, 150, 50, gfx_hex(0x00ff00));
	gfx_rect_xy(&da, 50, 210, 150, 50, gfx_hex(0x0000ff));
}


/* Compositing */

static void demo_2(void)
{
	gfx_color fb2[GFX_WIDTH * GFX_HEIGHT];
	struct gfx_drawable da2;

	gfx_da_init(&da2, GFX_WIDTH, GFX_HEIGHT, fb2);
	gfx_clear(&da2, GFX_BLACK);
	gfx_rect_xy(&da2, 100, 0, 50, GFX_HEIGHT, GFX_WHITE);

	gfx_color fb3[(GFX_WIDTH - 30) * 10];

	struct gfx_drawable da3;

	gfx_da_init(&da3, GFX_WIDTH - 30, 10, fb3);
	gfx_clear(&da3, GFX_CYAN);

	show_citrine();

#if 1
	gfx_copy(&da, 0, 0, &da2, 0, 0, GFX_WIDTH, GFX_HEIGHT, GFX_BLACK);
	gfx_copy(&da, 10, 150, &da3, 0, 0, GFX_WIDTH - 30, 10, -1);
#endif
}


/* Polygons */

static void demo_3(void)
{
	const short v[] = { 10, 10, 110, 10, 10, 210 };
	const short v2[] = { 100, 270, 230, 250, 150, 100 };

	gfx_poly(&da, 3, v, GFX_MAGENTA);
	gfx_poly(&da, 3, v2, GFX_YELLOW);
}


/* Text */

static void demo_4(bool new)
{
	const struct font *fonts[] = {
		&mono18,
		&mono28,
		&mono38,
	};
	unsigned i;

	for (i = 0; i != 3; i++)
		if (new)
			ntext_char(&da, 100, 50 + 50 * i, fonts[i], '5',
			    GFX_WHITE);
		else
			gfx_char(&da, 100, 50 + 50 * i, 16 << i, 16 << i, '@',
			    GFX_WHITE);
}


/* Horizontally scrolling text */


#define	DEMO_5_HEIGHT	32
#define	DEMO_5_HOLD_MS	500
#define	DEMO_5_DELAY_MS	10
#define	DEMO_5_STEP	2


static void demo_5(void)
{
	struct long_text lt;
	unsigned i = 10;

	long_text_setup(&lt, &da, 0, (GFX_HEIGHT - DEMO_5_HEIGHT) / 2,
	    GFX_WIDTH, DEMO_5_HEIGHT, "THIS IS A SCROLLING TEXT.",
	    DEMO_5_HEIGHT, GFX_WHITE, GFX_BLUE);
	while (i) {
		bool hold;

		t0();
		hold = long_text_scroll(&lt, &da, -DEMO_5_STEP);
		update_display(&da);
		t1("scroll & update");
		if (hold) {
			msleep(DEMO_5_HOLD_MS - DEMO_5_DELAY_MS);
			i--;
		}
		msleep(DEMO_5_DELAY_MS);
	}
}


/* Vertical scrolling (only on real hw) */


#define	DEMO_6_TOP	50
#define	DEMO_6_BOTTOM	80
#define	DEMO_6_MIDDLE	(GFX_HEIGHT - DEMO_6_TOP - DEMO_6_BOTTOM)
#define	DEMO_6_SCROLL	10
#define	DEMO_6_MARK	10


static void demo_6_pattern(void)
{
	unsigned x;

	gfx_rect_xy(&da, 0, 0, GFX_WIDTH, DEMO_6_TOP, GFX_GREEN);
	gfx_rect_xy(&da, 0, DEMO_6_TOP + DEMO_6_MIDDLE, GFX_WIDTH,
	    DEMO_6_BOTTOM, GFX_CYAN);
	for (x = 0; x != DEMO_6_TOP; x++)
		if (x & 1)
			gfx_rect_xy(&da, x, x, DEMO_6_MARK, 1, GFX_BLACK);
	for (x = 0; x != DEMO_6_MIDDLE; x++)
		if (x & 1)
			gfx_rect_xy(&da, x, DEMO_6_TOP + x, DEMO_6_MARK, 1,
			    GFX_WHITE);
	for (x = 0; x != DEMO_6_BOTTOM; x++)
		if (x & 1)
			gfx_rect_xy(&da, x, DEMO_6_TOP + DEMO_6_MIDDLE + x,
			    DEMO_6_MARK, 1, GFX_BLACK);
	gfx_rect_xy(&da, 0, DEMO_6_TOP + DEMO_6_SCROLL, GFX_WIDTH, 1,
	    GFX_RED);
}


static void demo_6(void)
{
	demo_6_pattern();
#ifndef SIM
	st7789_vscroll(DEMO_6_TOP, DEMO_6_TOP + DEMO_6_MIDDLE - 1,
	    DEMO_6_TOP + DEMO_6_SCROLL);
#endif /* !SIM */
}


static void demo_6a(unsigned y0, unsigned y1, unsigned ytop)
{
	demo_6_pattern();
#ifndef SIM
	st7789_vscroll(y0, y1, ytop);
#endif /* !SIM */
}


static void demo_6b(unsigned tfa, unsigned vsa, unsigned bfa, unsigned vsp)
{
	demo_6_pattern();
#ifndef SIM
	st7789_vscroll_raw(tfa, vsa, bfa, vsp);
#endif /* !SIM */
}


/* Text entry */


static bool demo_7_validate(const char *s)
{
	return !strchr(s, 'x');
}


/* --- Initialization ------------------------------------------------------ */


bool app_init(char *const *args, unsigned n_args)
{
	int param = 0;

	gfx_da_init(&da, GFX_WIDTH, GFX_HEIGHT, fb);
	gfx_clear(&da, gfx_hex(0));

	timer_init(&idle_timer);

	if (n_args) {
		char *end;

		param = strtoul(args[0], &end, 0);
		if (*end)
			return 0;
	}
	if (param)
		display_on(1);
	switch (param) {
	case 0:
		ui_switch(&ui_off);
		break;
	case 1:
		demo_1();
		break;
	case 2:
		demo_2();
		break;
	case 3:
		demo_3();
		break;
	case 4:
		demo_4(n_args > 1);
		break;
	case 5:
		demo_5();
		break;
	case 6:
		switch (n_args) {
		case 1:
			demo_6();
			break;
		case 4:
			demo_6a(atoi(args[1]), atoi(args[2]), atoi(args[3]));
			break;
		case 5:
			demo_6b(atoi(args[1]), atoi(args[2]), atoi(args[3]),
			    atoi(args[4]));
			break;
		default:
			return 0;
		}
		break;
	case 7:
		ui_entry_validate = demo_7_validate;
		ui_switch(&ui_entry);
		break;
	default:
		return 0;
	}

	update_display(&da);
	return 1;
}
