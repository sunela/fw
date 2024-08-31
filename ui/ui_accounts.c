/*
 * ui_accounts.c - User interface: show account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "wi_list.h"
#include "ui_overlay.h"
#include "ui_entry.h"
#include "db.h"
#include "style.h"
#include "ui.h"
#include "ui_accounts.h"


#define	NULL_TARGET_COLOR	GFX_HEX(0x808080)

struct ui_accounts_ctx {
	void (*resume_action)(struct ui_accounts_ctx *c);
	struct wi_list list;
	char buf[MAX_NAME_LEN + 1];
};


static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.entry = {
		.fg	= { GFX_WHITE, GFX_WHITE },
		.bg	= { GFX_BLACK, GFX_HEX(0x202020) },
		.min_h	= 50,
	}
};

static struct wi_list_entry_style style_null_target;
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


static int validate_new_account(void *user, const char *s)
{
	return db_iterate(&main_db, name_is_different, (void *) s);
}


static void make_new_account(struct ui_accounts_ctx *c,
    void (*call)(const struct ui *ui, void *user))
{
	struct ui_entry_params params = {
		.input = {
			.buf		= c->buf,
			.max_len	= sizeof(c->buf) - 1,
			.validate	= validate_new_account,
			.title		= "New account",
		},
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


/* --- Moving accounts ----------------------------------------------------- */


/*
 * Note: move_from and move_to receive a pointer to the db_entry on which the
 * overlay was invoked in "user", not the ui_accounts_ctx the other callbacks
 * get.
 */

static struct db_entry *moving = NULL;


static void move_from(void *user)
{
	assert(!moving);
	moving = user;
	ui_return();
}


static void move_to(void *user)
{
	struct db_entry *e = user;

	assert(moving);
	db_move_before(moving, e);
	moving = NULL;
	ui_return();
}


static void move_cancel(void *user)
{
	assert(moving);
	moving = NULL;
	ui_return();
}


void ui_accounts_cancel_move(void)
{
	moving = NULL;
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


static void remote_control(void *user)
{
	ui_switch(&ui_rmt, NULL);
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
	struct ui_accounts_ctx *c = ctx;

	if (y < LIST_Y0) {
		long_top(ctx, x, y);
		return;
	}

	static struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_power,	power_off, NULL },
		{ ui_overlay_sym_add,	new_account, NULL },
		{ ui_overlay_sym_move_from, move_from, NULL, },
		{ ui_overlay_sym_setup,	enter_setup, NULL },
		{ ui_overlay_sym_pc_comm, remote_control, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 5,
        };
	const struct wi_list_entry *entry = wi_list_pick(&c->list, x, y);
	unsigned i;

	for (i = 0; i != prm.n_buttons; i++)
		buttons[i].user = ctx;
	if (moving) {
		if (entry) {
			buttons[1].draw = ui_overlay_sym_move_to;
			buttons[1].fn = move_to;
			buttons[1].user = wi_list_user(entry);
		} else {
			buttons[1].draw = NULL;
		}
		buttons[2].draw = ui_overlay_sym_move_cancel;
		buttons[2].fn = move_cancel;
	} else {
		buttons[1].draw = ui_overlay_sym_add;
		buttons[1].fn = new_account;
		if (entry) {
			buttons[2].draw = ui_overlay_sym_move_from;
			buttons[2].fn = move_from;
			buttons[2].user = wi_list_user(entry);
		} else {
			buttons[2].draw = NULL;
		}
	}

	ui_call(&ui_overlay, &prm);
}


/* --- Open/close ---------------------------------------------------------- */


static bool add_account(void *user, struct db_entry *de)
{
	struct ui_accounts_ctx *c = user;
	struct wi_list_entry *e;

	e = wi_list_add(&c->list, de->name, NULL, de);
	if (moving && (de == moving || moving->next == de))
		wi_list_entry_style(&c->list, e, &style_null_target);
	return 1;
}


static void ui_accounts_open(void *ctx, void *params)
{
	struct ui_accounts_ctx *c = ctx;

	style_null_target = style.entry;
	style_null_target.fg[0] = style_null_target.fg[1] = NULL_TARGET_COLOR;

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
