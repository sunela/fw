/*
 * ui_account.c - User interface: show account details
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "alloc.h"
#include "fmt.h"
#include "base32.h"
#include "hotp.h"
#include "gfx.h"
#include "shape.h"
#include "text.h"
#include "db.h"
#include "wi_list.h"
#include "ui_overlay.h"
#include "ui_confirm.h"
#include "ui_entry.h"
#include "ui_field.h"
#include "ui_accounts.h"
#include "style.h"
#include "colors.h"
#include "ui.h"


#define	TITLE_FG		GFX_YELLOW
#define	TIMER_FG		GFX_HEX(0x8080ff)


struct ui_account_ctx {
	struct db_entry *selected_account;
	struct wi_list list;
	void (*resume_action)(struct ui_account_ctx *c);
	char buf[MAX_NAME_LEN + 1];
	int64_t last_tick;
	struct db_field *field_ref;	/* for passing a field with context */
};

static void render_account(const struct wi_list *l,
    const struct wi_list_entry *entry, struct gfx_drawable *d,
    const struct gfx_rect *bb, bool odd);


static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.entry = {
		.fg	= { LIST_FG, LIST_FG },
		.bg	= { EVEN_BG, ODD_BG },
		.min_h	= 50,
		.render	= render_account,
	}
};

static struct wi_list *lists[1];


/* --- Extra account rendering --------------------------------------------- */


static void render_account(const struct wi_list *l,
    const struct wi_list_entry *entry, struct gfx_drawable *d,
    const struct gfx_rect *bb, bool odd)
{
	struct db_field *f = wi_list_user(entry);
	unsigned passed_s = (time_us() / 1000000 + 1) % 30;

	if (!f || f->type != ft_totp_secret)
		return;
	gfx_arc(d, bb->x + bb->w - 1 - bb->h / 2, bb->y + bb->h / 2,
	    bb->h / 4, 12 * passed_s, 0,
	    passed_s ? TIMER_FG : style.entry.bg[odd], style.entry.bg[odd]);
}


static void show_totp(struct wi_list *l,
    struct wi_list_entry *entry, void *user)
{
	struct db_field *f = wi_list_user(entry);

	if (!f || f->type != ft_totp_secret)
		return;
	assert(f->len > 0);

	uint64_t counter = (time_us() + time_offset) / 1000000 / 30;
	uint32_t code;
	char s[6 + 1];
	char *p = s;

	code = hotp64(f->data, f->len, counter);
	format(add_char, &p, "%06u", (unsigned) code % 1000000);
	wi_list_update_entry(l, entry, "TOTP", s, f);
	wi_list_render_entry(l, entry);
	ui_update_display();
}


static void ui_account_tick(void *ctx)
{
	struct ui_account_ctx *c = ctx;
	int64_t this_tick = time_us() / 1000000;

	if (c->last_tick == this_tick)
		return;
	c->last_tick = this_tick;
	/*
	 * @@@ we update every second for the "time left" display. The code
	 * changes only every 30 seconds.
	 */

	wi_list_forall(&c->list, show_totp, NULL);
}


/* --- Tap ----------------------------------------------------------------- */


static void find_counter(struct wi_list *list, struct wi_list_entry *entry,
    void *user)
{
	struct wi_list_entry **res = user;
	struct db_field *f = wi_list_user(entry);

	if (f->type == ft_hotp_counter)
		*res = entry;
}


static void set_counter_entry(struct ui_account_ctx *c, struct db_field *f)
{
	struct wi_list_entry *entry = NULL;
	uint64_t counter;
	char s[10+1];
	char *p = s;

	wi_list_forall(&c->list, find_counter, &entry);
	if (entry) {
		assert(!f);
		f = wi_list_user(entry);
	} else {
		assert(f);
	}
	assert(f->len == sizeof(counter));
	memcpy(&counter, f->data, sizeof(counter));
	format(add_char, &p, "%u", (unsigned) counter);
	if (entry)
		wi_list_update_entry(&c->list, entry, "Counter", s, f);
	else
		wi_list_add(&c->list, "Counter", s, f);
}


static void ui_account_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_account_ctx *c = ctx;
	struct wi_list_entry *entry;
	struct db_field *f;
	const struct db_field *hotp_secret = NULL;
	unsigned hotp_secret_len;
	struct db_field *hotp_counter = NULL;
	uint64_t counter;

	char s[6 + 1];
	char *p = s;
	uint32_t code;

	if (list_is_empty(&c->list)) {
		if (button_in(GFX_WIDTH / 2, (GFX_HEIGHT + LIST_Y0) / 2, x, y))
			ui_call(&ui_field_add, c->selected_account);
		return;
	}

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;
	for (f = c->selected_account->fields; f; f = f->next)
		switch (f->type) {
		case ft_hotp_secret:
			hotp_secret = f->data;
			hotp_secret_len = f->len;
			break;
		case ft_hotp_counter:
			hotp_counter = f->data;
			assert(f->len == sizeof(counter));
			break;
		default:
			break;
		}
	if (!hotp_secret || !hotp_counter)
		return;

	progress();

	f = wi_list_user(entry);
	if (f->type != ft_hotp_secret)
		return;

	memcpy(&counter, hotp_counter, sizeof(counter));
	code = hotp64(hotp_secret, hotp_secret_len, counter);
	format(add_char, &p, "%06u", (unsigned) code % 1000000);
	wi_list_update_entry(&c->list, entry, "HOTP", s, f);

	counter++;
	if (!db_change_field(c->selected_account, ft_hotp_counter,
	    &counter, sizeof(counter)))
		debug("HOTP counter increment failed\n");

	set_counter_entry(c, NULL);

	ui_update_display();
}


/* --- Edit account name --------------------------------------------------- */


/*
 * @@@ Not great: we have account name editing both here and in ui_accounts.c
 */


static void changed_account_name(struct ui_account_ctx *c)
{
	if (!*c->buf || !strcmp(c->buf, c->selected_account->name))
		return;
	db_change_field(c->selected_account, ft_id, c->buf, strlen(c->buf));
	/* @@@ handle errors */
}


static bool name_is_different(void *user, struct db_entry *de)
{
	struct ui_account_ctx *c = user;

	return de == c->selected_account || strcmp(de->name, c->buf);
}


static int validate_name_change(void *user, const char *s)
{
	struct ui_account_ctx *c = user;

	/*
	 * We don't use "s" but instead c->buf. Not very pretty, but it keeps
	 * things simple.
	 */
	return db_iterate(&main_db, name_is_different, c);
}


static void edit_account_name(void *user)
{
	struct ui_account_ctx *c = user;

	struct ui_entry_params params = {
		.input = {
			.buf		= c->buf,
			.max_len	= sizeof(c->buf) - 1,
			.validate	= validate_name_change,
			.user		= c,
			.title		= "Account name",
		},
	};

	strcpy(c->buf, c->selected_account->name);
	c->resume_action = changed_account_name;
	ui_switch(&ui_entry, &params);
}


/* --- Long press ---------------------------------------------------------- */


static void power_off(void *user)
{
	turn_off();
}


static void confirm_entry_deletion(void *user, bool confirm)
{
	struct ui_account_ctx *c = user;

	if (confirm) {
		db_delete_entry(c->selected_account);
		c->selected_account = NULL;
		ui_accounts_cancel_move();
	}
}


static void delete_account(void *user)
{
	struct ui_account_ctx *c = user;

	struct ui_confirm_params prm = {
		.action	= "remove",
		.name	= c->selected_account->name,
		.fn	= confirm_entry_deletion,
		.user	= c,
	};

	ui_switch(&ui_confirm, &prm);
}


static void account_overlay(struct ui_account_ctx *c)
{
	static struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_power,	power_off, NULL },
		{ ui_overlay_sym_edit,	edit_account_name, NULL },
		{ ui_overlay_sym_delete, delete_account, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 3,
	};
	unsigned i;

	for (i = 0; i != prm.n_buttons; i++)
		buttons[i].user = c;
	ui_call(&ui_overlay, &prm);
}


static void add_field(void *user)
{
	struct ui_account_ctx *c = user;

	ui_switch(&ui_field_add, c->selected_account);
}


static void edit_field(void *user)
{
	struct ui_account_ctx *c = user;
	struct db_field *f = c->field_ref;
	struct ui_field_edit_params prm = {
		.de	= c->selected_account,
		.type	= f->type,
	};

	ui_switch(&ui_field_edit, &prm);
}


static void confirm_field_deletion(void *user, bool confirm)
{
	struct ui_account_ctx *c = user;
	struct db_entry *de = c->selected_account;
	struct db_field *f = c->field_ref;
	struct db_field *f2 = NULL;

	if (!confirm)
		return;
	if (f->type == ft_pw)
		f2 = db_field_find(de, ft_pw2);
	if (f2) {
		db_entry_defer_update(de, 1);
		db_change_field(de, ft_pw, f2->data, f2->len);
		db_delete_field(de, f2);
		db_entry_defer_update(de, 0);
		// @@@ handle errors
	} else {
		db_delete_field(de, f);
		// @@@ handle errors
	}
}


static void delete_field(void *user)
{
	struct ui_account_ctx *c = user;
	struct db_field *f = c->field_ref;
	struct ui_confirm_params prm = {
		.action	= "remove field",
		.fn	= confirm_field_deletion,
		.user	= c,
	};

	switch (f->type) {
	case ft_user:
		prm.name = "user name";
		break;
	case ft_email:
		prm.name = "e-mail";
		break;
	case ft_pw:
		prm.name = "password";
		break;
	case ft_pw2:
		prm.name = "2nd password";
		break;
	case ft_hotp_secret:
		prm.name = "HOTP";
		break;
	case ft_totp_secret:
		prm.name = "TOTP";
		break;
	case ft_comment:
		prm.name = "comment";
		break;
	default:
		ABORT();
	}
	ui_switch(&ui_confirm, &prm);
}


static void fields_overlay(struct ui_account_ctx *c, struct db_field *f)
{
	static struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_power,	power_off, NULL },
		{ ui_overlay_sym_add,	add_field, NULL },
		{ ui_overlay_sym_edit,	edit_field, NULL },
		{ ui_overlay_sym_delete, delete_field, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 0,
	};
	struct db_entry *de = c->selected_account;
	unsigned i;

	if (ui_field_more(de)) {
		buttons[1].draw = ui_overlay_sym_add;
		prm.n_buttons = f ? 4 : 2;
	} else {
		buttons[1].draw = NULL;
		prm.n_buttons = f ? 4 : 1;
	}
	c->field_ref = f;
	for (i = 0; i != prm.n_buttons; i++)
		buttons[i].user = c;
	ui_call(&ui_overlay, &prm);
}


static void ui_account_long(void *ctx, unsigned x, unsigned y)
{
	struct ui_account_ctx *c = ctx;

	if (y < LIST_Y0) {
		account_overlay(c);
	} else {
		struct wi_list_entry *entry = wi_list_pick(&c->list, x, y);

		fields_overlay(c, entry ? wi_list_user(entry) : NULL);
	}
}


/* --- Open/close ---------------------------------------------------------- */


static void add_string(struct ui_account_ctx *c, const char *label,
    const char *s, unsigned len, void *user)
{
	char *tmp = alloc_size(len + 1);

	memcpy(tmp, s, len);
	tmp[len] = 0;
	wi_list_add(&c->list, label, tmp, user);
	free(tmp);
}


static void ui_account_open(void *ctx, void *params)
{
	struct ui_account_ctx *c = ctx;
	struct db_entry *de = params;
	struct db_field *f, *f2;
	unsigned i;

	c->selected_account = de;
	c->resume_action = NULL;
	c->last_tick = -1;

	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, de->name, &FONT_TOP,
	    GFX_CENTER, GFX_CENTER, TITLE_FG);

	wi_list_begin(&c->list, &style);
	for (i = 0; i != field_types; i++) {
		f = db_field_find(de, order2ft[i]);
		if (!f)
			continue;
		switch (f->type) {
		case ft_id:
		case ft_prev:
		case ft_dir:
			break;
		case ft_user:
			add_string(c, "User", f->data, f->len, f);
			break;
		case ft_email:
			add_string(c, "E-Mail", f->data, f->len, f);
			break;
		case ft_pw:
			f2 = db_field_find(de, ft_pw2);
			if (f2) {
				add_string(c, "Password 1", f->data, f->len, f);
				add_string(c, "Password 2", f2->data, f2->len,
				    f2);
			} else {
				add_string(c, "Password", f->data, f->len, f);
			}
			break;
		case ft_pw2:
			break;
		case ft_hotp_secret:
			wi_list_add(&c->list, "HOTP", "------", f);
			break;
		case ft_hotp_counter:
			set_counter_entry(c, f);
			break;
		case ft_totp_secret:
			wi_list_add(&c->list, "TOTP", "------", f);
			break;
		case ft_comment:
			add_string(c, "Comment", f->data, f->len, f);
			break;
		default:
			ABORT();
		}
	}
	wi_list_end(&c->list);

	if (list_is_empty(&c->list)) {
		button_draw_add(GFX_WIDTH / 2, (GFX_HEIGHT + LIST_Y0) / 2);
		lists[0] = NULL;
	} else {
		lists[0] = &c->list;
	}

	set_idle(IDLE_ACCOUNT_S);
}


static void ui_account_close(void *ctx)
{
	struct ui_account_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


static void ui_account_resume(void *ctx)
{
	struct ui_account_ctx *c = ctx;

	ui_account_close(ctx);
	if (c->resume_action)
		c->resume_action(c);
	if (c->selected_account)
		ui_account_open(ctx, c->selected_account);
	else
		ui_return();
	progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_account_events = {
	.touch_tap	= ui_account_tap,
	.touch_long	= ui_account_long,
	.touch_to	= swipe_back,
	.tick		= ui_account_tick,
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_account = {
	.name		= "account",
	.ctx_size	= sizeof(struct ui_account_ctx),
	.open		= ui_account_open,
	.close		= ui_account_close,
	.resume		= ui_account_resume,
	.events		= &ui_account_events,
};
