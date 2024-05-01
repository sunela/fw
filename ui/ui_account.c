/*
 * ui_account.c - User interface: show account details
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "hal.h"
#include "fmt.h"
#include "hotp.h"
#include "gfx.h"
#include "text.h"
#include "accounts.h"
#include "ui_list.h"
#include "ui.h"


#define	FONT_TOP		mono18

#define	TOP_H			30
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)

#define	HOLD_MS	(5 * 1000)


static struct account *selected_account = NULL;

static const struct ui_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
};

static struct ui_list list;


/* --- Event handling ------------------------------------------------------ */


static void ui_account_tap(unsigned x, unsigned y)
{
	struct ui_list_entry *entry;
	struct account *a;
	char s[6 + 1];
	char *p = s;
	uint32_t code;

	entry = ui_list_pick(&list, x, y);
	if (!entry)
		return;
	a = ui_list_user(entry);
	if (!a)
		return;
	if (!a->token.secret_size || a->token.type != tt_hotp)
		return;

	/* @@@ make it harder to update the counter ? */
	a->token.counter++;
	code = hotp64(a->token.secret, a->token.secret_size, a->token.counter);
	format(add_char, &p, "%06u", (unsigned) code % 1000000);
	ui_list_update_entry(&list, entry, "HOTP", s, a);
	update_display(&da);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_account_open(void *params)
{
	struct account *a = selected_account = params;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, a->name, &FONT_TOP,
	    GFX_CENTER, GFX_CENTER, GFX_YELLOW);

	ui_list_begin(&list, &style);
	if (a->user)
		ui_list_add(&list, "User", a->user, NULL);
	if (a->pw)
		ui_list_add(&list, "Password", a->pw, NULL);
	if (a->token.secret_size) {
		switch (a->token.type) {
		case tt_hotp:
			ui_list_add(&list, "HOTP", "------", a);
			break;
		default:
			abort();
		}
	}
	ui_list_end(&list);
}


static void ui_account_close(void)
{
	ui_list_destroy(&list);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_account_events = {
	.touch_tap	= ui_account_tap,
};

const struct ui ui_account = {
	.open = ui_account_open,
	.close = ui_account_close,
	.events = &ui_account_events,
};
