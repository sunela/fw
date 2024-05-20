/*
 * ui_accounts.c - User interface: show account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <string.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "wi_list.h"
#include "ui_overlay.h"
#include "ui_entry.h"
#include "db.h"
#include "style.h"
#include "ui.h"


struct ui_accounts_ctx {
	void (*resume_action)(struct ui_accounts_ctx *c);
	struct wi_list list;
	char buf[MAX_NAME_LEN + 1];
};


static const struct wi_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
	min_h:	50,
};

static struct wi_list *lists[1];


/* --- New account --------------------------------------------------------- */


static void new_account_name(struct ui_accounts_ctx *c)
{
	if (!*c->buf)
		return;
	/* @@@ handle errors */
	db_new_entry(&main_db, c->buf);
}


static bool name_is_different(void *user, struct db_entry *de)
{
	const char *s = user;

	return strcmp(de->name, s);
}


static bool validate_new_account(void *user, const char *s)
{
	return db_iterate(&main_db, name_is_different, (void *) s);
}


static void make_new_account(struct ui_accounts_ctx *c,
    void (*call)(const struct ui *ui, void *user))
{
	struct ui_entry_params params = {
		.buf		= c->buf,
		.max_len	= sizeof(c->buf) - 1,
		.validate	= validate_new_account,
		.title		= "New account",
	};

	*c->buf = 0;
	c->resume_action = new_account_name;
	call(&ui_entry, &params);
}


static void new_account(void *user)
{
	struct ui_accounts_ctx *c = user;

	make_new_account(c, ui_switch);
}


/* --- Tap event ----------------------------------------------------------- */


static void ui_accounts_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_accounts_ctx *c = ctx;
	const struct wi_list_entry *entry;

	if (list_is_empty(&c->list)) {
		if (button_in(GFX_WIDTH / 2, (GFX_HEIGHT + LIST_Y0) / 2, x, y))
			make_new_account(c, ui_call);
		return;
	}

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;
	ui_call(&ui_account, wi_list_user(entry));
}


/* --- Long press event ---------------------------------------------------- */


static void power_off(void *user)
{
	turn_off();
}


static void enter_setup(void *user)
{
	ui_switch(&ui_setup, NULL);
}


static void long_top(void *ctx, unsigned x, unsigned y)
{
	/* @@@ future: if in sub-folder, edit folder name */
	static struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_power,	power_off, NULL },
		{ ui_overlay_sym_setup,	enter_setup, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 2,
        };
	unsigned i;

	for (i = 0; i != prm.n_buttons; i++)
		buttons[i].user = ctx;
	ui_call(&ui_overlay, &prm);
}


static void ui_accounts_long(void *ctx, unsigned x, unsigned y)
{
	if (y < LIST_Y0) {
		long_top(ctx, x, y);
		return;
	}

	static struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_power,	power_off, NULL },
		{ ui_overlay_sym_add,	new_account, NULL },
		{ NULL, },
		{ ui_overlay_sym_setup,	enter_setup, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 4,
        };
	unsigned i;

	for (i = 0; i != prm.n_buttons; i++)
		buttons[i].user = ctx;
	ui_call(&ui_overlay, &prm);
}


/* --- Open/close ---------------------------------------------------------- */


static bool add_account(void *user, struct db_entry *de)
{
	struct ui_accounts_ctx *c = user;

	wi_list_add(&c->list, de->name, NULL, de);
	return 1;
}


static void ui_accounts_open(void *ctx, void *params)
{
	struct ui_accounts_ctx *c = ctx;

	lists[0] = &c->list;
	c->resume_action = NULL;

	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, "Accounts",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&c->list, &style);
	db_iterate(&main_db, add_account, c);
	wi_list_end(&c->list);

	if (list_is_empty(&c->list))
		button_draw_add(GFX_WIDTH / 2, (GFX_HEIGHT + LIST_Y0) / 2);

	set_idle(IDLE_ACCOUNTS_S);
}


static void ui_accounts_close(void *ctx)
{
	struct ui_accounts_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


static void ui_accounts_resume(void *ctx)
{
	struct ui_accounts_ctx *c = ctx;

	/*
	 * @@@ once we have vertical scrolling, we'll also need to restore the
	 * position.
	 */
	ui_accounts_close(ctx);
	if (c->resume_action)
		c->resume_action(c);
	ui_accounts_open(ctx, NULL);
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
	.name		= "accounts",
	.ctx_size	= sizeof(struct ui_accounts_ctx),
	.open		= ui_accounts_open,
	.close		= ui_accounts_close,
	.resume		= ui_accounts_resume,
	.events		= &ui_accounts_events,
};
