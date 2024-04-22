/*
 * ui_accounts.c - User interface: show account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "hal.h"
#include "gfx.h"
#include "ntext.h"
#include "ui_list.h"
#include "accounts.h"
#include "ui_account.h"
#include "ui.h"


#define	FONT_TOP		mono18

#define	TOP_H			30
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)

#define	HOLD_MS	(5 * 1000)


static const struct ui_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
};

static struct ui_list list;


/* --- Event handling ------------------------------------------------------ */


static void ui_accounts_tap(unsigned x, unsigned y)
{
	const struct ui_list_entry *entry;

	entry = ui_list_pick(&list, x, y);
	if (!entry)
		return;
	selected_account = ui_list_user(entry);
	ui_switch(&ui_account);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_accounts_open(void)
{
	unsigned i;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	ntext_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Accounts",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	ui_list_begin(&list, &style);
	for (i = 0; i != n_accounts; i++)
		ui_list_add(&list, accounts[i].name, NULL, accounts + i);
	ui_list_end(&list);
}


static void ui_accounts_close(void)
{
	ui_list_destroy(&list);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_accounts_events = {
	.touch_tap	= ui_accounts_tap,
};

const struct ui ui_accounts = {
	.open = ui_accounts_open,
        .close = ui_accounts_close,
	.events = &ui_accounts_events,
};
