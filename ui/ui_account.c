/*
 * ui_account.c - User interface: show account details
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "hal.h"
#include "hotp.h"
#include "gfx.h"
#include "ntext.h"
#include "accounts.h"
#include "ui_account.h"
#include "ui_list.h"
#include "ui.h"


#define	FONT_TOP_SIZE		22
#define	FONT_TOP		mono18

#define	TOP_BG			gfx_hex(0x30ff50)

#define	TOP_H			30
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)

#define	HOLD_MS	(5 * 1000)


struct account *selected_account = NULL;

static const struct ui_list_style style = {
	y0:	0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
};

static struct ui_list list;


/* --- Event handling ------------------------------------------------------ */


static void ui_account_tap(unsigned x, unsigned y)
{
        const struct ui_list_entry *entry;

        entry = ui_list_pick(&list, x, y);
        if (!entry)
                return;
	/* for revealing HOTP codes */
}

/* --- Open/close ---------------------------------------------------------- */


static void ui_account_open(void)
{
	struct account *a = selected_account;

	ui_list_begin(&list, &style);
	ui_list_add(&list, a->name, NULL, NULL);
	if (a->user) {
		ui_list_add(&list, "User", NULL, NULL);
		ui_list_add(&list, a->user, NULL, NULL);
	}
	if (a->pw) {
		ui_list_add(&list, "Password", NULL, NULL);
		ui_list_add(&list, a->pw, NULL, NULL);
	}
	if (a->token.secret_size) {
		/* @@@ ugly. should have list entries that strdup */
		static char s[6 + 1];
		uint32_t code;

		switch (a->token.type) {
		case tt_hotp:
			code = hotp64(a->token.secret, a->token.secret_size,
			    a->token.counter);
			sprintf(s, "%06u", (unsigned) code);
			ui_list_add(&list, "HOTP", s, NULL);
			/* @@@ increment needs to be user-triggered */
			a->token.counter++;
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
