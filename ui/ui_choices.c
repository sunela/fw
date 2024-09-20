/*
 * ui_choices.c - User interface: List of page choices
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "wi_list.h"
#include "style.h"
#include "ui.h"
#include "ui_choices.h"


struct ui_choices_ctx {
	struct wi_list list;
	struct ui_choices_params params;
	bool call;
};


static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.entry = {
		.fg	= { GFX_WHITE, GFX_WHITE },
		.bg	= { GFX_BLACK, GFX_HEX(0x202020) },
		.min_h	= 50,
	}
};

static struct wi_list *lists[1];


/* --- Event handling ------------------------------------------------------ */


static void ui_choices_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_choices_ctx *c = ctx;
	const struct wi_list_entry *entry;
	const struct ui_choice *choice;

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;
	choice = wi_list_user(entry);
	if (!choice)
		return;
	if (c->params.call)
		ui_call(choice->ui, choice->params);
	else
		ui_switch(choice->ui, choice->params);
}


static void ui_choices_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_choices_open(void *ctx, void *params)
{
	struct ui_choices_ctx *c = ctx;
	struct ui_choices_params *prm = params;
	unsigned i;

	lists[0] = &c->list;
	c->params = *prm;

	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, prm->title,
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&c->list, &style);
	for (i = 0; i != prm->n_choices; i++) {
		const struct ui_choice *choice = prm->choices + i;

		wi_list_add(&c->list, choice->label, NULL, (void *) choice);
	}
	wi_list_end(&c->list);
}


static void ui_choices_close(void *ctx)
{
	struct ui_choices_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


static void ui_choices_resume(void *ctx)
{
	struct ui_choices_ctx *c = ctx;
	struct ui_choices_params params = c->params;

        /*
         * @@@ once we have vertical scrolling, we'll also need to restore the
         * position.
         */
        ui_choices_close(ctx);
        ui_choices_open(ctx, &params);
        progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_choices_events = {
	.touch_tap	= ui_choices_tap,
	.touch_to	= ui_choices_to,
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_choices = {
	.name		= "choices",
	.ctx_size	= sizeof(struct ui_choices_ctx),
	.open		= ui_choices_open,
	.close		= ui_choices_close,
	.resume		= ui_choices_resume,
	.events		= &ui_choices_events,
};
