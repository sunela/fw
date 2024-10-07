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
#include "shape.h"
#include "text.h"
#include "wi_list.h"
#include "ui_overlay.h"
#include "ui_confirm.h"
#include "ui_entry.h"
#include "db.h"
#include "style.h"
#include "ui.h"
#include "ui_accounts.h"


#define	NULL_TARGET_COLOR	GFX_HEX(0x808080)
#define	TITLE_ROOT_FG		GFX_WHITE
#define	TITLE_SUB_FG		GFX_YELLOW


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
static struct wi_list_entry_style style_dir;
static struct wi_list *lists[1];


/* --- New entry ----------------------------------------------------------- */


static void new_entry_name(struct ui_accounts_ctx *c)
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


static int validate_new_entry(void *user, const char *s)
{
	return db_iterate(&main_db, name_is_different, (void *) s);
}


static void make_new_entry(struct ui_accounts_ctx *c,
    void (*call)(const struct ui *ui, void *user))
{
	struct ui_entry_params params = {
		.input = {
			.buf		= c->buf,
			.max_len	= sizeof(c->buf) - 1,
			.validate	= validate_new_entry,
			.title		= "New entry",
		},
	};

	*c->buf = 0;
	c->resume_action = new_entry_name;
	call(&ui_entry, &params);
}


static void new_entry(void *user)
{
	struct ui_accounts_ctx *c = user;

	make_new_entry(c, ui_switch);
}


/* --- Tap event ----------------------------------------------------------- */


static void ui_accounts_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_accounts_ctx *c = ctx;
	const struct wi_list_entry *entry;
	struct db_entry *de;

	if (list_is_empty(&c->list)) {
		if (button_in(GFX_WIDTH / 2, (GFX_HEIGHT + LIST_Y0) / 2, x, y))
			make_new_entry(c, ui_call);
		return;
	}

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;
	de = wi_list_user(entry);
	if (db_is_dir(de)) {
		db_chdir(&main_db, de);
		ui_switch(&ui_accounts, NULL);
	} else {
		ui_call(&ui_account, de);
	}
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


/* --- Swipe --------------------------------------------------------------- */


static void ui_accounts_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe != us_left)
		return;
	db_chdir(&main_db, db_dir_parent(&main_db));
	ui_switch(&ui_accounts, NULL);
}


/* --- Edit the directory name --------------------------------------------- */


/*
 * @@@ ui_account.c has almost identical code for editing the name of account
 * entries.
 */

static void changed_dir_name(struct ui_accounts_ctx *c)
{
	if (!*c->buf || !strcmp(c->buf, db_pwd(&main_db)))
                return;
	db_rename(main_db.dir, c->buf);
	/* @@@ handle errors */
}


static bool dir_name_is_different(void *user, struct db_entry *de)
{
	struct ui_accounts_ctx *c = user;

	return de == main_db.dir || strcmp(de->name, c->buf);
}


static int validate_dir_name_change(void *user, const char *s)
{
	struct ui_accounts_ctx *c = user;

	/*
	 * We don't use "s" but instead c->buf. Not very pretty, but it keeps
	 * things simple.
	 */
	return db_iterate(&main_db, dir_name_is_different, c);
}


static void edit_dir_name(void *user)
{
	struct ui_accounts_ctx *c = user;

	struct ui_entry_params params = {
		.input = {
			.buf		= c->buf,
			.max_len	= sizeof(c->buf) - 1,
			.validate	= validate_dir_name_change,
			.user		= c,
			.title		= "Directory name",
		},
	};

	strcpy(c->buf, db_pwd(&main_db));
	c->resume_action = changed_dir_name;
	ui_switch(&ui_entry, &params);
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


static void confirm_dir_deletion(void *user, bool confirm)
{
        if (confirm) {
		struct db_entry *parent = db_dir_parent(&main_db);

		db_delete_entry(main_db.dir);
		main_db.dir = parent;
		ui_accounts_cancel_move();
		ui_switch(&ui_accounts, NULL);
	}
}


static void delete_dir(void *user)
{
	struct ui_accounts_ctx *c = user;

	struct ui_confirm_params prm = {
		.action	= "remove",
		.name	= db_pwd(&main_db),
		.fn	= confirm_dir_deletion,
		.user	= c,
        };

	ui_switch(&ui_confirm, &prm);
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
		{ ui_overlay_sym_edit, edit_dir_name, NULL },
		{ ui_overlay_sym_delete, delete_dir, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		/* .n_buttons is set below */
	};
	unsigned i;

	prm.n_buttons = db_pwd(&main_db) ? main_db.dir->children ? 3 : 4 : 2;
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
		{ ui_overlay_sym_setup,	enter_setup, NULL },
		{ ui_overlay_sym_pc_comm, remote_control, NULL },
		{ ui_overlay_sym_add, new_entry, NULL },
		{ ui_overlay_sym_move_from, move_from, NULL, },
		{ NULL, move_cancel, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 6,
	};
	const struct wi_list_entry *entry = wi_list_pick(&c->list, x, y);
	unsigned i;

	for (i = 0; i != prm.n_buttons; i++)
		buttons[i].user = ctx;
	if (moving) {
		if (entry) {
			buttons[4].draw = ui_overlay_sym_move_to;
			buttons[4].fn = move_to;
			buttons[4].user = wi_list_user(entry);
		} else {
			buttons[4].draw = NULL;
		}
		buttons[5].draw = ui_overlay_sym_move_cancel;
	} else {
		if (entry) {
			buttons[4].draw = ui_overlay_sym_move_from;
			buttons[4].fn = move_from;
			buttons[4].user = wi_list_user(entry);
		} else {
			buttons[4].draw = NULL;
		}
		buttons[5].draw = NULL;
	}

	ui_call(&ui_overlay, &prm);
}


/* --- Folder icon --------------------------------------------------------- */


static void draw_folder(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned w, int8_t align_x, int8_t align_y, gfx_color fg, gfx_color bg)
{
	unsigned h = w * 2 / 3;

	switch (align_x) {
	case GFX_LEFT:
		break;
	case GFX_RIGHT:
		x -= w - 1;
		break;
	default:
		ABORT();
	}
	switch (align_y) {
	case GFX_TOP:
		break;
	case GFX_CENTER:
		y -= h / 2 + h / 5;
		break;
	case GFX_BOTTOM:
		y -= h - 1;
		break;
	default:
		ABORT();
	}
	gfx_folder_outline(da, x, y, w, h,
	    w / 2, h / 5, h / 9, 3,	// rider_w, rider_h, r, lw
	    fg, bg);
}


static void render_folder(const struct wi_list *list,
    const struct wi_list_entry *entry, struct gfx_drawable *da,
    const struct gfx_rect *bb, bool odd)
{
	// leave a 1 px margin on the right
	draw_folder(da, bb->x + bb->w - 1 - 1, bb->y + bb->h / 2, bb->h / 2,
	    GFX_RIGHT, GFX_CENTER, style.entry.fg[odd], style.entry.bg[odd]);
}


/* --- Open/close ---------------------------------------------------------- */


static bool add_account(void *user, struct db_entry *de)
{
	struct ui_accounts_ctx *c = user;
	struct wi_list_entry *e;

	if (db_is_dir(de)) {
		e = wi_list_add_width(&c->list, de->name, NULL,
		    GFX_WIDTH - style_dir.min_h - 2, de);
		wi_list_entry_style(&c->list, e, &style_dir);
	} else {
		e = wi_list_add(&c->list, de->name, NULL, de);
		if (moving && (de == moving || moving->next == de))
			wi_list_entry_style(&c->list, e, &style_null_target);
	}
	return 1;
}


#define	TOP_CORNER_X	10


static void ui_accounts_open(void *ctx, void *params)
{
	struct ui_accounts_ctx *c = ctx;
	const char *pwd;
	style_null_target = style.entry;
	style_null_target.fg[0] = style_null_target.fg[1] = NULL_TARGET_COLOR;
	style_dir = style.entry;
	style_dir.render = render_folder;

	c->resume_action = NULL;

	pwd = db_pwd(&main_db);
	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	if (pwd) {
		text_text(&main_da, (GFX_WIDTH - TOP_CORNER_X - TOP_H) / 2,
		    TOP_H / 2, pwd,
		    &FONT_TOP, GFX_CENTER, GFX_CENTER, TITLE_SUB_FG);
		draw_folder(&main_da, GFX_WIDTH - TOP_CORNER_X - 1, TOP_H / 2,
		    TOP_H / 2, GFX_RIGHT, GFX_CENTER, TITLE_SUB_FG, GFX_BLACK);
	} else {
		text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, "Accounts",
		    &FONT_TOP, GFX_CENTER, GFX_CENTER, TITLE_ROOT_FG);
	}

	wi_list_begin(&c->list, &style);
	db_iterate(&main_db, add_account, c);
	wi_list_end(&c->list);

	if (list_is_empty(&c->list)) {
		button_draw_add(GFX_WIDTH / 2, (GFX_HEIGHT + LIST_Y0) / 2);
		lists[0] = NULL;
	} else {
		lists[0] = &c->list;
	}

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
	.touch_to	= ui_accounts_to,
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
