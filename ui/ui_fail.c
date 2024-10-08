/*
 * ui_fail.c - User interface: incorrect PIN and cooldown
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <assert.h>

#include "hal.h"
#include "timer.h"
#include "gfx.h"
#include "text.h"
#include "pin.h"
#include "ui.h"


#define	FONT		mono58


static struct timer t_tick;


/* --- Show cooldown timer (in seconds) ------------------------------------ */


static void show_cooldown(void *user)
{
	static struct gfx_rect bb = { 0, 0, 0, 0 };
	unsigned s = (pin_cooldown_ms() + 999) / 1000;
	char buf[5];
	char *t = buf + sizeof(buf);

	if (!s) {
		ui_switch(&ui_pin, NULL);
		return;
	}
	assert(s < 10000);
	*--t = 0;
	while (s) {
		*--t = '0' + s % 10;
		s /= 10;
	}
	if (user)
		gfx_rect(&main_da, &bb, GFX_BLACK);

	text_text(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2, t, &FONT,
	    GFX_CENTER | GFX_MAX, GFX_CENTER | GFX_MAX, GFX_RED);
	text_bbox(GFX_WIDTH / 2, GFX_HEIGHT / 2, t, &FONT,
	    GFX_CENTER | GFX_MAX, GFX_CENTER | GFX_MAX, &bb);
	ui_update_display();

	timer_set(&t_tick, 1000, show_cooldown, show_cooldown);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_fail_open(void *ctx, void *params)
{
	timer_init(&t_tick);
	if (!pin_cooldown_ms()) {
		ui_switch(&ui_pin, NULL);
		return;
	}
	set_idle(IDLE_HOLD_S);
	show_cooldown(NULL);
}


static void ui_cooldown_open(void *ctx, void *params)
{
	timer_init(&t_tick);
	set_idle(IDLE_HOLD_S);
	show_cooldown(NULL);
}


static void ui_fail_close(void *ctx)
{
	timer_cancel(&t_tick);
}


const struct ui ui_fail = {
	.name	= "fail",
	.open	= ui_fail_open,
	.close	= ui_fail_close,
};


const struct ui ui_cooldown = {
	.open = ui_cooldown_open,
	.close = ui_fail_close,
};
