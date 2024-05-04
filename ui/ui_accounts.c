/*
 * ui_accounts.c - User interface: show account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "uw_list.h"
#include "ut_overlay.h"
#include "accounts.h"
#include "ui.h"


#define	FONT_TOP		mono18

#define	TOP_H			30
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)


static const struct uw_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
};

static struct uw_list list;


/* --- Tap event ----------------------------------------------------------- */


static void ui_accounts_tap(unsigned x, unsigned y)
{
	const struct uw_list_entry *entry;

	entry = uw_list_pick(&list, x, y);
	if (!entry)
		return;
	ui_call(&ui_account, uw_list_user(entry));
}


/* --- Long press event ---------------------------------------------------- */


static void power_off(void *user)
{
	turn_off();
}


static void add_account(void *user)
{
	// @@@
}


static void enter_setup(void *user)
{
	ui_switch(&ut_setup, NULL);
}


static void ui_accounts_long(unsigned x, unsigned y)
{
	/* @@@ future: if in sub-folder, edit folder name */
	if (y < LIST_Y0)
		return;

	static const struct ut_overlay_button buttons[] = {
		{ ui_overlay_sym_power,	power_off, NULL },
		{ ui_overlay_sym_add,	add_account, NULL },
		{ NULL, },
		{ ui_overlay_sym_setup,	enter_setup, NULL },
	};
	struct ut_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 4,
        };

	ui_call(&ut_overlay, &prm);
	
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_accounts_open(void *params)
{
	unsigned i;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Accounts",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	uw_list_begin(&list, &style);
	for (i = 0; i != n_accounts; i++)
		uw_list_add(&list, accounts[i].name, NULL, accounts + i);
	uw_list_end(&list);

	set_idle(IDLE_ACCOUNTS_S);
}


static void ui_accounts_close(void)
{
	uw_list_destroy(&list);
}


static void ui_accounts_resume(void)
{
	/*
	 * @@@ once we have vertical scrolling, we'll also need to restore the
	 * position.
	 */
	ui_accounts_close();
	ui_accounts_open(NULL);
	progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_accounts_events = {
	.touch_tap	= ui_accounts_tap,
	.touch_long	= ui_accounts_long,
};

const struct ui ui_accounts = {
	.open = ui_accounts_open,
	.close = ui_accounts_close,
	.resume = ui_accounts_resume,
	.events = &ui_accounts_events,
};
