/*
 * ut_setup.c - User interface tool: Setup
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "uw_list.h"
#include "accounts.h"
#include "ui.h"


#define	FONT_TOP		mono24

#define	TOP_H			31
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)


static const struct uw_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
};

static struct uw_list list;


/* --- Event handling ------------------------------------------------------ */


static void ut_setup_tap(unsigned x, unsigned y)
{
	const struct uw_list_entry *entry;
	const struct ui *next;

	entry = uw_list_pick(&list, x, y);
	if (!entry)
		return;
	next = uw_list_user(entry);
	if (!next)
		return;
	ui_call(next, NULL);
}


static void ut_setup_to(unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ut_setup_open(void *params)
{
	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Setup",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	uw_list_begin(&list, &style);
	uw_list_add(&list, "Change PIN", NULL, NULL);
	uw_list_add(&list, "Time & date", NULL, (void *) &ut_time);
	uw_list_end(&list);

	set_idle(IDLE_SETUP_S);
}


static void ut_setup_close(void)
{
	uw_list_destroy(&list);
}


static void ut_setup_resume(void)
{
        /*
         * @@@ once we have vertical scrolling, we'll also need to restore the
         * position.
         */
        ut_setup_close();
        ut_setup_open(NULL);
        progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ut_setup_events = {
	.touch_tap	= ut_setup_tap,
	.touch_to	= ut_setup_to,
};

const struct ui ut_setup = {
	.open = ut_setup_open,
	.close = ut_setup_close,
	.resume = ut_setup_resume,
	.events = &ut_setup_events,
};
