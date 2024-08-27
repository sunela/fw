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


#define	FG		GFX_BLACK
#define	BG		GFX_WHITE

#define	FONT		mono18

#define	BOX_R		10
#define	BOX_MARGIN	BOX_R



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
	unsigned w = p->w ? p->w : GFX_WIDTH;
	unsigned h;

	assert(w > 2 * BOX_MARGIN);
	h = text_format(NULL, 0, 0, w - 2 * BOX_MARGIN, 0, 0, p->s,
	    &FONT, GFX_CENTER, FG);
	gfx_rrect_xy(&main_da, (GFX_WIDTH - w) / 2,
	    (GFX_HEIGHT - h) / 2 - BOX_MARGIN,
	    w, h + 2 * BOX_MARGIN, BOX_R, BG);
	text_format(&main_da, (GFX_WIDTH - w) / 2 + BOX_MARGIN,
	    (GFX_HEIGHT - h) / 2, w - 2 * BOX_MARGIN, h, 0, p->s,
	    &FONT, GFX_CENTER, FG);
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
