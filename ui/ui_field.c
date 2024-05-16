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


static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.fg	= { FIELD_FG, FIELD_FG },
	.bg	= { EVEN_BG, ODD_BG },
	.min_h	= 40,
};

static struct wi_list list;
static struct wi_list *lists[1] = { &list };
static void (*edit_resume_action)(void) = NULL;


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

/*
 * @@@ Parameter passing is extremely clumsy here. We really need ui.c to
 * maintain a stack of context buffers for the pages in use.
 */

static char buf[MAX_STRING_LEN + 1];
static struct ui_field_edit_params edited_params;


#define	PARAMS(text, max, fn)			\
	do {					\
		entry_params.title = (text);	\
		entry_params.max_len = (max);	\
		entry_params.validate = (fn);	\
	} while (0)


static void field_edited(void)
{
	uint8_t tmp[MAX_SECRET_LEN];
	ssize_t len;
	char *end;
	uint64_t counter;

	if (!*buf) {
		ui_return();
		return;
	}
	switch (edited_params.type) {
	case ft_user:
	case ft_email:
	case ft_pw:
		/* @@@ report errors */
		db_change_field(edited_params.de, edited_params.type,
		    buf, strlen(buf));
		break;
	case ft_hotp_secret:
	case ft_totp_secret:
		len = base32_decode(tmp, MAX_SECRET_LEN, buf);
		db_change_field(edited_params.de, edited_params.type, tmp, len);
		break;
	case ft_hotp_counter:
		counter = strtoull(buf, &end, 10);
		assert(!*end);	// @@@ should check properly :)
		db_change_field(edited_params.de, edited_params.type, &counter,
		    sizeof(counter));
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
	const struct ui_field_edit_params *p = params;
	struct db_field *f;

	edit_resume_action = NULL;
	for (f = p->de->fields; f; f = f->next)
		if (f->type == p->type)
			break;

	struct ui_entry_params entry_params = {
		.buf		= buf,
		/* we set the remaining fields below */
	};

	switch (p->type) {
	case ft_user:
		PARAMS("User name", MAX_STRING_LEN, NULL);
		copy_value(buf, f);	
		break;
	case ft_email:
		PARAMS("E-Mail", MAX_STRING_LEN, NULL);
		copy_value(buf, f);	
		break;
	case ft_pw:
		PARAMS("Password", MAX_STRING_LEN, NULL);
		copy_value(buf, f);	
		break;
	case ft_hotp_secret:
		PARAMS("HOTP Secret", MAX_STRING_LEN, validate_base32);
		copy_base32(buf, f);
		break;
	case ft_hotp_counter:
		PARAMS("Counter", MAX_STRING_LEN, validate_decimal);
		copy_base32(buf, f);
		break;
	case ft_totp_secret:
		PARAMS("TOTP Secret", MAX_STRING_LEN, validate_base32);
		copy_base32(buf, f);
		break;
	default:
		abort();
	}
	edit_resume_action = field_edited;
	edited_params = *p;
	ui_call(&ui_entry, &entry_params);
}


static void ui_field_edit_close(void *ctx)
{
	/* for now, nothing to do */
}


static void ui_field_edit_resume(void *ctx)
{
	ui_field_edit_close(ctx);
	if (edit_resume_action)
		edit_resume_action();
        progress();
}


/* --- Selection of type for new field ------------------------------------- */


static void ui_field_add_tap(void *ctx, unsigned x, unsigned y)
{
	const struct wi_list_entry *entry;

	entry = wi_list_pick(&list, x, y);
	if (!entry)
		return;
	ui_switch(&ui_field_edit, wi_list_user(entry));
}


static void ui_field_add_open(void *ctx, void *params)
{
	static struct ui_field_edit_params ctx_user = { type: ft_user, };
	static struct ui_field_edit_params ctx_email = { type: ft_email, };
	static struct ui_field_edit_params ctx_pw = { type: ft_pw, };
	static struct ui_field_edit_params ctx_hotp_secret =
	    { type: ft_hotp_secret, };
	static struct ui_field_edit_params ctx_totp_secret =
	    { type: ft_totp_secret, };
	struct db_entry *de = params;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Field type",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	ctx_user.de = ctx_email.de = ctx_pw.de =
	    ctx_hotp_secret.de = ctx_totp_secret.de = de;
	wi_list_begin(&list, &style);
	if (!have(de, ft_user))
		wi_list_add(&list, "User", NULL, &ctx_user);
	if (!have(de, ft_email))
		wi_list_add(&list, "E-Mail", NULL, &ctx_email);
	if (!have(de, ft_pw))
		wi_list_add(&list, "Password", NULL, &ctx_pw);
	if (!have(de, ft_hotp_secret) && !have(de, ft_totp_secret)) {
		wi_list_add(&list, "HOTP", NULL, &ctx_hotp_secret);
		wi_list_add(&list, "TOTP", NULL, &ctx_totp_secret);
	}
	wi_list_end(&list);
}


static  void ui_field_add_close(void *ctx)
{
	wi_list_destroy(&list);
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_field_edit = {
	.open = ui_field_edit_open,
	.close = ui_field_edit_close,
	.resume = ui_field_edit_resume,
};

static const struct ui_events ui_field_add_events = {
	.touch_tap	= ui_field_add_tap,
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_field_add = {
	.open = ui_field_add_open,
	.close = ui_field_add_close,
	.events = &ui_field_add_events,
};
