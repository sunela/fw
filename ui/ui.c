/*
 * ui.c - User interface: setup and event distribution
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "hal.h"
#include "timer.h"
#include "gfx.h"
#include "pin.h"
#include "ui.h"


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

static void demo_4(void)
{
	unsigned i;

	for (i = 0; i != 3; i++)
		gfx_char(&da, 100, 50 + 50 * i, 16 << i, 16 << i, '@',
		    GFX_WHITE);
}


/* --- Initialization ------------------------------------------------------ */


void app_init(int param)
{
	gfx_da_init(&da, GFX_WIDTH, GFX_HEIGHT, fb);
	gfx_clear(&da, gfx_hex(0));

	timer_init(&idle_timer);

	switch (param) {
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
		demo_4();
		break;
	default:
		ui_switch(&ui_off);
		// @@@
		break;
	}

	update_display(&da);
}
