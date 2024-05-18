/*
 * ui_confirm.c - User interface: Confirm an action
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "hal.h"
#include "debug.h"
#include "gfx.h"
#include "text.h"
#include "wi_list.h"
#include "ui.h"
#include "ui_confirm.h"


#define	TEXT_COLOR		GFX_WHITE
#define	NAME_COLOR		GFX_YELLOW
#define	CHEVRONS_PASSIVE_COLOR	GFX_WHITE
#define	CHEVRONS_ACTIVE_COLOR	GFX_YELLOW

#define	TEXT_FONT		mono18
#define	NAME_FONT		mono24

#define	TEXT_Y0			50
#define	Y_STEP			30

#define	CHEVRONS_Y		(TEXT_Y0 + 3 * Y_STEP + CHEVRONS_R + 20)
#define	CHEVRONS_R		20
#define	CHEVRONS_W		5
#define	CHEVRONS_X_STEP		20


static void (*cb)(void *user, bool confirm);
static void *cb_user;
static int current_x;


/* --- Helper functions ---------------------------------------------------- */


static bool in_band(unsigned y)
{
	return y >= CHEVRONS_Y - CHEVRONS_R && y <= CHEVRONS_Y + CHEVRONS_R;
}


/* --- Draw chevrons ------------------------------------------------------- */


static void chevrons(unsigned x0, unsigned x1, gfx_color color)
{
	unsigned x, d;

	for (x = CHEVRONS_R; x + CHEVRONS_W <= GFX_WIDTH;
	    x += CHEVRONS_X_STEP)
		for (d = 0; d != CHEVRONS_R; d++) {
			unsigned from = x - d < x0 ? x0 : x - d;
			unsigned to = x - d + CHEVRONS_W >= x1 ? x1 :
			    x - d + CHEVRONS_W;

			if (from <= to) {
				unsigned w = to - from + 1;

				gfx_rect_xy(&da, from, CHEVRONS_Y - d, w, 1,
				    color);
				gfx_rect_xy(&da, from, CHEVRONS_Y + d, w, 1,
				    color);
			}
		}
}


/* --- Event handling ------------------------------------------------------ */


static void ui_confirm_tap(void *ctx, unsigned x, unsigned y)
{
	debug("ui_confirm_tap\n");
	if (cb)
		cb(cb_user, 0);
	ui_return();
}


static void ui_confirm_moving(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (!in_band(from_y) || !in_band(to_y))
		return;
	if (current_x < 0)
		current_x = from_x;
	if ((unsigned) current_x == to_x)
		return;
	chevrons(current_x, to_x, CHEVRONS_ACTIVE_COLOR);
	update_display(&da);
}


static void ui_confirm_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	debug("ui_confirm_to %u in %u-%u\n",
	    from_y, CHEVRONS_Y - CHEVRONS_R, CHEVRONS_Y + CHEVRONS_R);
	if (cb)
		cb(cb_user, swipe == us_right &&
		    in_band(from_y) && in_band(to_y));
	ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_confirm_open(void *ctx, void *params)
{
	const struct ui_confirm_params *p = params;
	unsigned y = TEXT_Y0;

	text_text(&da, GFX_WIDTH / 2, y, "Swipe right to", &TEXT_FONT,
	    GFX_CENTER, GFX_ORIGIN, TEXT_COLOR);
	y += Y_STEP;
	text_text(&da, GFX_WIDTH / 2, y, p->action, &TEXT_FONT,
	    GFX_CENTER, GFX_ORIGIN, TEXT_COLOR);
	y += Y_STEP;
	text_text(&da, GFX_WIDTH / 2, y, p->name, &NAME_FONT,
	    GFX_CENTER, GFX_ORIGIN, NAME_COLOR);

	chevrons(0, GFX_WIDTH - 1, CHEVRONS_PASSIVE_COLOR);
	current_x = -1;

	cb = p->fn;
	cb_user = p->user;
	set_idle(IDLE_CONFIRM_S);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_confirm_events = {
	.touch_tap	= ui_confirm_tap,
	.touch_moving	= ui_confirm_moving,
	.touch_to	= ui_confirm_to,
};

const struct ui ui_confirm = {
	.name	= "confirm",
	.open	= ui_confirm_open,
	.events	= &ui_confirm_events,
};
