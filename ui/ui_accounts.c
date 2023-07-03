/*
 * ui_accounts.c - User interface: show account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "hal.h"
#include "gfx.h"
#include "ui.h"


#define	HOLD_MS	(5 * 1000)

#define	FONT_SIZE	24


/* --- Open/close ---------------------------------------------------------- */


static void ui_accounts_open(void)
{
	gfx_text(&da, GFX_WIDTH / 2, GFX_HEIGHT / 2, "Accounts", FONT_SIZE,
	    GFX_CENTER, GFX_CENTER, GFX_WHITE);
}


static void ui_accounts_close(void)
{
}


const struct ui ui_accounts = {
	.open = ui_accounts_open,
        .close = ui_accounts_close,
};
