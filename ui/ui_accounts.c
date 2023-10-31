/*
 * ui_accounts.c - User interface: show account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "hal.h"
#include "gfx.h"
#include "ui_list.h"
#include "accounts.h"
#include "ui.h"


#define	HOLD_MS	(5 * 1000)



static struct ui_list list;


/* --- Open/close ---------------------------------------------------------- */


static void ui_accounts_open(void)
{
	unsigned i;

	ui_list_begin(&list);
	for (i = 0; i != n_accounts; i++)
		ui_list_add(&list, accounts[i].name, (void *) accounts + i);
	ui_list_end(&list);
}


static void ui_accounts_close(void)
{
	ui_list_destroy(&list);
}


const struct ui ui_accounts = {
	.open = ui_accounts_open,
        .close = ui_accounts_close,
};
