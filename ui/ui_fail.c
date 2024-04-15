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
#include "ntext.h"
#include "pin.h"
#include "ui.h"


#define	HOLD_MS	(5 * 1000)

#define	FONT_SIZE	72
#define	FONT		mono58


static struct timer t_hold;
static struct timer t_tick;


/* --- Show cooldown timer (in seconds) ------------------------------------ */


static void show_cooldown(void *user)
{
	static struct gfx_rect bb = { 0, 0, 0, 0 };
	unsigned s = pin_cooldown < now ? 0 : (pin_cooldown - now) / 1000;
	char buf[5];
	char *t = buf + sizeof(buf);

	if (pin_cooldown < now) {
		ui_switch(&ui_pin);
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

	if (use_ntext) {
		ntext_text(&da, GFX_WIDTH / 2, GFX_HEIGHT / 2, t, &FONT,
		    GFX_CENTER, GFX_CENTER, GFX_RED);
		ntext_text_bbox(GFX_WIDTH / 2, GFX_HEIGHT / 2, t, &FONT,
		    GFX_CENTER, GFX_CENTER, &bb);
	} else {
		gfx_text(&da, GFX_WIDTH / 2, GFX_HEIGHT / 2, t, FONT_SIZE,
		    GFX_CENTER, GFX_CENTER, GFX_RED);
		gfx_text_bbox(GFX_WIDTH / 2, GFX_HEIGHT / 2, t, FONT_SIZE,
		    GFX_CENTER, GFX_CENTER, &bb);
	}
	update_display(&da);

	timer_set(&t_tick, 1000, show_cooldown, show_cooldown);
}


static void end_hold(void *user)
{
	turn_off();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_fail_open(void)
{
	timer_init(&t_hold);
	timer_init(&t_tick);
	pin_attempts++;
	if (pin_attempts < PIN_FREE_ATTEMPTS) {
		ui_switch(&ui_pin);
		return;
	}
	timer_set(&t_hold, HOLD_MS, end_hold, NULL);
	pin_cooldown = now + PIN_WAIT_S(pin_attempts) * 1000;
	show_cooldown(NULL);
}


static void ui_cooldown_open(void)
{
	timer_init(&t_hold);
	timer_init(&t_tick);
	timer_set(&t_hold, HOLD_MS, end_hold, NULL);
	show_cooldown(NULL);
}


static void ui_fail_close(void)
{
	timer_cancel(&t_hold);
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
