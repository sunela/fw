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
	unsigned s = pin_cooldown < now ? 0 : (pin_cooldown - now) / 1000;
	char buf[5];
	char *t = buf + sizeof(buf);

	if (pin_cooldown < now) {
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
		gfx_rect(&da, &bb, GFX_BLACK);

	text_text(&da, GFX_WIDTH / 2, GFX_HEIGHT / 2, t, &FONT,
	    GFX_CENTER, GFX_CENTER, GFX_RED);
	text_text_bbox(GFX_WIDTH / 2, GFX_HEIGHT / 2, t, &FONT,
	    GFX_CENTER, GFX_CENTER, &bb);
	update_display(&da);

	timer_set(&t_tick, 1000, show_cooldown, show_cooldown);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_fail_open(void *ctx, void *params)
{
	timer_init(&t_tick);
	pin_attempts++;
	if (pin_attempts < PIN_FREE_ATTEMPTS) {
		ui_switch(&ui_pin, NULL);
		return;
	}
	set_idle(IDLE_HOLD_S);
	pin_cooldown = now + PIN_WAIT_S(pin_attempts) * 1000;
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
	.open = ui_fail_open,
	.close = ui_fail_close,
};


const struct ui ui_cooldown = {
	.open = ui_cooldown_open,
	.close = ui_fail_close,
};
