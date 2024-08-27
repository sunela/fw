/*
 * ui_notice.c - User interface: Show a notice
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "shape.h"
#include "text.h"
#include "ui.h"
#include "ui_notice.h"


#define	DEFAULT_BOX_R	10
#define	DEFAULT_FONT	mono18


static const struct ui_notice_style default_style = {
	.fg		= GFX_BLACK,
	.bg		= GFX_WHITE,
	.x_align	= GFX_CENTER,
	.font		= &mono18,
};


/* --- Event handling ------------------------------------------------------ */


static void ui_notice_tap(void *ctx, unsigned x, unsigned y)
{
	ui_return();
}


static void ui_notice_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
        if (swipe == us_left)
                ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_notice_open(void *ctx, void *params)
{
	const struct ui_notice_params *p = params;
	const struct ui_notice_style *style =
	    p->style ? p->style : &default_style;
	unsigned w = p->w ? p->w : GFX_WIDTH;
	unsigned r = p->r ? p->r : DEFAULT_BOX_R;
	unsigned margin = p->margin ? p->margin : r;
	const struct font *font = style->font ? style->font : &DEFAULT_FONT;
	unsigned h;

	assert(w > 2 * margin);
	h = text_format(NULL, 0, 0, w - 2 * margin, 0, 0, p->s,
	    font, style->x_align, style->fg);
	gfx_rrect_xy(&main_da, (GFX_WIDTH - w) / 2,
	    (GFX_HEIGHT - h) / 2 - margin,
	    w, h + 2 * margin, r, style->bg);
	text_format(&main_da, (GFX_WIDTH - w) / 2 + margin,
	    (GFX_HEIGHT - h) / 2, w - 2 * margin, h, 0, p->s,
	    font, style->x_align, style->fg);
	set_idle(IDLE_NOTICE_S);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_notice_events = {
	.touch_tap	= ui_notice_tap,
	.touch_to	= ui_notice_to,
};

const struct ui ui_notice = {
	.name		= "notice",
	.open		= ui_notice_open,
	.events		= &ui_notice_events,
};
