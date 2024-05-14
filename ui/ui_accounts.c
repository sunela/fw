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
#include "wi_list.h"
#include "ui_overlay.h"
#include "ui.h"


#define	FONT_TOP		mono18

#define	TOP_H			30
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)


static const struct wi_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
	min_h:	40,
};

static struct wi_list list;
static struct wi_list *lists[1] = { &list };



/* --- Tap event ----------------------------------------------------------- */


static void ui_accounts_tap(unsigned x, unsigned y)
{
	const struct wi_list_entry *entry;

	entry = wi_list_pick(&list, x, y);
	if (!entry)
		return;
	ui_call(&ui_account, wi_list_user(entry));
}


/* --- Long press event ---------------------------------------------------- */


static void power_off(void *user)
{
	turn_off();
}


static void new_account(void *user)
{
	// @@@
}


static void enter_setup(void *user)
{
	ui_switch(&ui_setup, NULL);
}


static void ui_accounts_long(unsigned x, unsigned y)
{
	/* @@@ future: if in sub-folder, edit folder name */
	if (y < LIST_Y0)
		return;

	static const struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_power,	power_off, NULL },
		{ ui_overlay_sym_add,	new_account, NULL },
		{ NULL, },
		{ ui_overlay_sym_setup,	enter_setup, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 4,
        };

	ui_call(&ui_overlay, &prm);
	
}


/* --- Open/close ---------------------------------------------------------- */


static bool add_account(void *user, struct db_entry *de)
{
	wi_list_add(&list, de->name, NULL, de);
	return 1;
}


static void ui_accounts_open(void *params)
{
	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Accounts",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&list, &style);
	db_iterate(&main_db, add_account, NULL);
	wi_list_end(&list);

	set_idle(IDLE_ACCOUNTS_S);
}


static void ui_accounts_close(void)
{
	wi_list_destroy(&list);
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
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_accounts = {
	.open = ui_accounts_open,
	.close = ui_accounts_close,
	.resume = ui_accounts_resume,
	.events = &ui_accounts_events,
};
