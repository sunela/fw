/*
 * ui_setup.c - User interface: Setup
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "wi_list.h"
#include "ui.h"


#define	FONT_TOP		mono24

#define	TOP_H			31
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)


struct ui_setup_ctx {
	struct wi_list list;
};


static const struct wi_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
	min_h:	50,
};

static struct wi_list *lists[1];


/* --- Event handling ------------------------------------------------------ */


static void ui_setup_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_setup_ctx *c = ctx;
	const struct wi_list_entry *entry;
	const struct ui *next;

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;
	next = wi_list_user(entry);
	if (!next)
		return;
	ui_call(next, NULL);
}


static void ui_setup_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_setup_open(void *ctx, void *params)
{
	struct ui_setup_ctx *c = ctx;

	lists[0] = &c->list;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Setup",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&c->list, &style);
	wi_list_add(&c->list, "Change PIN", NULL, NULL);
	wi_list_add(&c->list, "Time & date", NULL, (void *) &ui_time);
	wi_list_add(&c->list, "Storage", NULL, (void *) &ui_storage);
	wi_list_end(&c->list);

	set_idle(IDLE_SETUP_S);
}


static void ui_setup_close(void *ctx)
{
	struct ui_setup_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


static void ui_setup_resume(void *ctx)
{
        /*
         * @@@ once we have vertical scrolling, we'll also need to restore the
         * position.
         */
        ui_setup_close(ctx);
        ui_setup_open(ctx, NULL);
        progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_setup_events = {
	.touch_tap	= ui_setup_tap,
	.touch_to	= ui_setup_to,
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_setup = {
	.name		= "setup",
	.ctx_size	= sizeof(struct ui_setup_ctx),
	.open		= ui_setup_open,
	.close		= ui_setup_close,
	.resume		= ui_setup_resume,
	.events		= &ui_setup_events,
};
