/*
 * ui_field.c - User interface: Field operations
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "base32.h"
#include "db.h"
#include "gfx.h"
#include "ui.h"
#include "wi_list.h"
#include "ui_overlay.h"
#include "ui_entry.h"
#include "ui_field.h"


#define	FONT_TOP		mono18

#define	TOP_H			30
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)

#define	FIELD_FG		GFX_WHITE
#define	EVEN_BG			GFX_BLACK
#define	ODD_BG			GFX_HEX(0x202020)


struct ui_field_edit_ctx {
	struct db_entry *de;
	enum field_type type;
	char buf[MAX_STRING_LEN + 1];
};

struct ui_field_add_ctx {
	struct wi_list list;
	struct db_entry *de;
};

static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.fg	= { FIELD_FG, FIELD_FG },
	.bg	= { EVEN_BG, ODD_BG },
	.min_h	= 50,
};

static struct wi_list *lists[1];


/* --- Determine what fields we can add ------------------------------------ */


static bool have(const struct db_entry *de, enum field_type type)
{
	const struct db_field *f;

	for (f = de->fields; f; f = f->next)
		if (f->type == type)
			return 1;
	return 0;
}


bool ui_field_more(const struct db_entry *de)
{
	if (!have(de, ft_user))
		return 1;
	if (!have(de, ft_email))
		return 1;
	if (!have(de, ft_pw))
		return 1;
	if (have(de, ft_hotp_secret))
		return !have(de, ft_hotp_counter);
	return !have(de, ft_totp_secret);
}


/* --- Create and/or edit a field ------------------------------------------ */


/*
 * @@@ HOTP/TOTP secrets need more work:
 * - the entry page should only accept characters from the base32 alphabet.
 * - we need to integrate alternative input options, e.g., USB.
 *
 * Also the HOTP counter needs more work:
 * - the entry page should only accept digits
 */

#define	PARAMS(text, max, fn)			\
	do {					\
		entry_params.title = (text);	\
		entry_params.max_len = (max);	\
		entry_params.validate = (fn);	\
	} while (0)


static void field_edited(struct ui_field_edit_ctx *c)
{
	uint8_t tmp[MAX_SECRET_LEN];
	ssize_t len;
	char *end;
	uint64_t counter;

	if (!*c->buf) {
		ui_return();
		return;
	}
	switch (c->type) {
	case ft_user:
	case ft_email:
	case ft_pw:
		/* @@@ report errors */
		db_change_field(c->de, c->type, c->buf, strlen(c->buf));
		break;
	case ft_hotp_secret:
	case ft_totp_secret:
		len = base32_decode(tmp, MAX_SECRET_LEN, c->buf);
		db_change_field(c->de, c->type, tmp, len);
		break;
	case ft_hotp_counter:
		counter = strtoull(c->buf, &end, 10);
		assert(!*end);	// @@@ should check properly :)
		db_change_field(c->de, c->type, &counter, sizeof(counter));
		break;
	default:
		abort();
	}
	ui_return();
}


static void copy_value(char *to, const struct db_field *from)
{
	if (from) {
		memcpy(to, from->data, from->len);
		to[from->len] = 0;
	} else {
		*to = 0;
	}
}


static void copy_base32(char *to, const struct db_field *from)
{
	if (from)
		base32_encode(to, base32_encode_size(MAX_SECRET_LEN),
		    from->data, from->len);
	else
		*to = 0;
}


static bool validate_base32(void *user, const char *s)
{
	return base32_decode_size(s) > 0;
}


static bool validate_decimal(void *user, const char *s)
{
	char *end;

	strtoull(s, &end, 10);
	return !*end;
}


static void ui_field_edit_open(void *ctx, void *params)
{
	struct ui_field_edit_ctx *c = ctx;
	const struct ui_field_edit_params *p = params;
	struct db_field *f;

	c->de = p->de;
	c->type = p->type;

	for (f = p->de->fields; f; f = f->next)
		if (f->type == p->type)
			break;

	struct ui_entry_params entry_params = {
		.buf		= c->buf,
		/* we set the remaining fields below */
	};

	switch (p->type) {
	case ft_user:
		PARAMS("User name", MAX_STRING_LEN, NULL);
		copy_value(c->buf, f);	
		break;
	case ft_email:
		PARAMS("E-Mail", MAX_STRING_LEN, NULL);
		copy_value(c->buf, f);	
		break;
	case ft_pw:
		PARAMS("Password", MAX_STRING_LEN, NULL);
		copy_value(c->buf, f);	
		break;
	case ft_hotp_secret:
		PARAMS("HOTP Secret", MAX_STRING_LEN, validate_base32);
		copy_base32(c->buf, f);
		break;
	case ft_hotp_counter:
		PARAMS("Counter", MAX_STRING_LEN, validate_decimal);
		copy_base32(c->buf, f);
		break;
	case ft_totp_secret:
		PARAMS("TOTP Secret", MAX_STRING_LEN, validate_base32);
		copy_base32(c->buf, f);
		break;
	default:
		abort();
	}
	ui_call(&ui_entry, &entry_params);
}


static void ui_field_edit_close(void *ctx)
{
	/* for now, nothing to do */
}


static void ui_field_edit_resume(void *ctx)
{
	struct ui_field_edit_ctx *c = ctx;

	ui_field_edit_close(ctx);
	field_edited(c);
        progress();
}


/* --- Selection of type for new field ------------------------------------- */


static void ui_field_add_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_field_add_ctx *c = ctx;
	const struct wi_list_entry *entry;

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;

	struct ui_field_edit_params prm = {
		.de = c->de,
		.type = (uintptr_t) wi_list_user(entry),
	};
	ui_switch(&ui_field_edit, &prm);
}


static void ui_field_add_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


static void ui_field_add_open(void *ctx, void *params)
{
	struct ui_field_add_ctx *c = ctx;

	lists[0] = &c->list;
	c->de = params;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Field type",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&c->list, &style);
	if (!have(c->de, ft_user))
		wi_list_add(&c->list, "User", NULL,
		    (void *) (uintptr_t) ft_user);
	if (!have(c->de, ft_email))
		wi_list_add(&c->list, "E-Mail", NULL,
		    (void *) (uintptr_t) ft_email);
	if (!have(c->de, ft_pw))
		wi_list_add(&c->list, "Password", NULL,
		    (void *) (uintptr_t) ft_pw);
	if (!have(c->de, ft_hotp_secret) && !have(c->de, ft_totp_secret)) {
		wi_list_add(&c->list, "HOTP", NULL,
		    (void *) (uintptr_t) ft_hotp_secret);
		wi_list_add(&c->list, "TOTP", NULL,
		    (void *) (uintptr_t) ft_totp_secret);
	}
	wi_list_end(&c->list);
}


static  void ui_field_add_close(void *ctx)
{
	struct ui_field_add_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_field_edit = {
	.name		= "field_edit",
	.ctx_size	= sizeof(struct ui_field_edit_ctx),
	.open		= ui_field_edit_open,
	.close		= ui_field_edit_close,
	.resume		= ui_field_edit_resume,
};

static const struct ui_events ui_field_add_events = {
	.touch_tap	= ui_field_add_tap,
	.touch_to	= ui_field_add_to,
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_field_add = {
	.name		= "field_add",
	.ctx_size	= sizeof(struct ui_field_add_ctx),
	.open		= ui_field_add_open,
	.close		= ui_field_add_close,
	.events		= &ui_field_add_events,
};
