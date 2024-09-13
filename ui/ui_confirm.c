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
#define	CHEVRONS_ACCEPT_COLOR	GFX_GREEN
#define	CHEVRONS_REJECT_COLOR	GFX_RED

#define	TEXT_FONT		mono18
#define	NAME_FONT		mono24
#define	NAME_FONT_SMALLER	mono18

#define	TEXT_Y0			50
#define	Y_STEP			30

#define	CHEVRONS_Y		(TEXT_Y0 + 3 * Y_STEP + CHEVRONS_R + 20)
#define	CHEVRONS_R		30
#define	CHEVRONS_W		5
#define	CHEVRONS_X_STEP		20


struct ui_confirm_ctx {
	void (*cb)(void *user, bool confirm);
	void *cb_user;
	int current_x;
	int last_decision;
};



/* --- Helper functions ---------------------------------------------------- */


static bool in_band(unsigned y)
{
	return y >= CHEVRONS_Y - CHEVRONS_R && y <= CHEVRONS_Y + CHEVRONS_R;
}


static int do_accept(unsigned from_y, unsigned to_y, enum ui_swipe swipe)
{
	if (!in_band(from_y))
		return 0;
	if (swipe != us_right)
		return 0;
	return in_band(to_y) ? 1 : -1;
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

				gfx_rect_xy(&main_da, from, CHEVRONS_Y - d,
				    w, 1, color);
				gfx_rect_xy(&main_da, from, CHEVRONS_Y + d,
				    w, 1, color);
			}
		}
}


/* --- Event handling ------------------------------------------------------ */


static void ui_confirm_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_confirm_ctx *c = ctx;

	debug("ui_confirm_tap\n");
	if (c->cb)
		c->cb(c->cb_user, 0);
	ui_return();
}


static void ui_confirm_moving(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	struct ui_confirm_ctx *c = ctx;
	int decision = do_accept(from_y, to_y, swipe);
	gfx_color color;

	if (!in_band(from_y))
		return;

	switch (decision) {
	case 0:
		color = CHEVRONS_ACTIVE_COLOR;
		break;
	case -1:
		color = CHEVRONS_REJECT_COLOR;
		break;
	case 1:
		color = CHEVRONS_ACCEPT_COLOR;
		break;
	default:
		ABORT();
	}

	if (decision == c->last_decision) {
		if (c->current_x < 0)
			c->current_x = from_x;
		if ((unsigned) c->current_x == to_x)
			return;
		chevrons(c->current_x, to_x, color);
	} else {
		chevrons(from_x, to_x, color);
		c->last_decision = decision;
	}
	if (c->current_x > (int) to_x)
		chevrons(to_x + 1, c->current_x,
		    CHEVRONS_PASSIVE_COLOR);
	c->current_x = to_x;
	ui_update_display();
}


static void ui_confirm_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	struct ui_confirm_ctx *c = ctx;

	debug("ui_confirm_to %u-%u in %u-%u\n",
	    from_y, to_y, CHEVRONS_Y - CHEVRONS_R, CHEVRONS_Y + CHEVRONS_R);
	if (c->cb)
		c->cb(c->cb_user, do_accept(from_y, to_y, swipe) == 1);
	ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_confirm_open(void *ctx, void *params)
{
	struct ui_confirm_ctx *c = ctx;
	const struct ui_confirm_params *p = params;
	unsigned y = TEXT_Y0;
	struct gfx_rect bb;

	text_text(&main_da, GFX_WIDTH / 2, y, "Swipe right to", &TEXT_FONT,
	    GFX_CENTER, GFX_ORIGIN, TEXT_COLOR);
	y += Y_STEP;
	text_text(&main_da, GFX_WIDTH / 2, y, p->action, &TEXT_FONT,
	    GFX_CENTER, GFX_ORIGIN, TEXT_COLOR);
	y += Y_STEP;

	text_bbox(0, 0, p->name, &NAME_FONT, GFX_LEFT, GFX_CENTER, &bb);
	if (bb.w <= GFX_WIDTH) {
		text_text(&main_da, GFX_WIDTH / 2, y, p->name, &NAME_FONT,
		    GFX_CENTER, GFX_ORIGIN, NAME_COLOR);
	} else {
		text_text(&main_da, GFX_WIDTH / 2, y, p->name,
		    &NAME_FONT_SMALLER, GFX_CENTER, GFX_ORIGIN, NAME_COLOR);
	}
	/* @@@ should also handle overflowing with NAME_FONT_SMALLER */

	chevrons(0, GFX_WIDTH - 1, CHEVRONS_PASSIVE_COLOR);
	c->current_x = -1;
	c->last_decision = 0;

	c->cb = p->fn;
	c->cb_user = p->user;
	set_idle(IDLE_CONFIRM_S);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_confirm_events = {
	.touch_tap	= ui_confirm_tap,
	.touch_moving	= ui_confirm_moving,
	.touch_to	= ui_confirm_to,
};

const struct ui ui_confirm = {
	.name		= "confirm",
	.ctx_size	= sizeof(struct ui_confirm_ctx),
	.open		= ui_confirm_open,
	.events		= &ui_confirm_events,
};
