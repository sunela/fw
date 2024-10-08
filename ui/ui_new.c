/*
 * ui_new.c - User interface: New device setup
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <string.h>

#include "pin.h"
#include "text.h"
#include "gfx.h"
#include "ui.h"
#include "settings.h"
#include "dbcrypt.h"
#include "db.h"
#include "wi_pin_entry.h"
#include "wi_general_entry.h"
#include "ui_notice.h"
#include "ui_entry.h"


enum stage {
	S_NEW		= 0,
	S_CONFIRM	= 1,
};

struct ui_new_ctx {
	enum stage stage;
	char buf[MAX_PIN_LEN + 1];
	struct wi_pin_entry_ctx pin_entry_ctx;
	struct wi_general_entry_ctx general_entry_ctx;
	uint32_t new_pin;
};


/* --- Helper functions ---------------------------------------------------- */


static int validate(void *user, const char *s)
{
	unsigned len = strlen(s);

	return len >= MIN_PIN_LEN && len <= MAX_PIN_LEN;
}


/* --- Entry --------------------------------------------------------------- */


static void entry(struct ui_new_ctx *c)
{
	static struct ui_entry_style style;
	struct ui_entry_params params = {
		.input = {
			.buf		= c->buf,
			.max_len	= MAX_PIN_LEN,
			.validate	= validate,
		},
		.style = &style,
		.maps = &ui_entry_decimal_maps,
		.entry_ops = &wi_general_entry_ops,
		.entry_user = &c->general_entry_ctx,
	};

	style = ui_entry_default_style;
	style.title_font = &mono18;
	memset(c->buf, 0, MAX_PIN_LEN + 1);
	switch (c->stage) {
	case S_NEW:
		wi_general_entry_setup(&c->general_entry_ctx, 1);
		params.input.title = "New device PIN";
		break;
	case S_CONFIRM:
		wi_general_entry_setup(&c->general_entry_ctx, 0);
		params.input.title = "Confirm PIN";
		break;
	default:
		ABORT();
	}
	progress();
	ui_call(&ui_entry, &params);
}


/* --- Set the new pin ----------------------------------------------------- */


static void write_new_settings(void)
{
	struct dbcrypt *crypt;

	crypt = dbcrypt_init(master_secret, sizeof(master_secret));
	if (!crypt) {
		notice_switch(&ui_off, NULL, nt_fault,
		    "Could not set up secrets");
		return;
	}
	if (!db_open(&main_db, crypt)) {
		notice_switch(&ui_off, NULL, nt_fault,
		    "Could not open database\n");
		return;
	}
	settings_reset();
	if (!settings_update()) {
		notice_switch(&ui_off, NULL, nt_fault,
		    "Could not store initial settings");
		return;
	}
	notice_switch(&ui_accounts, NULL, nt_success,
	    "PIN set");
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_new_open(void *ctx, void *params)
{
	struct ui_new_ctx *c = ctx;

	memset(c, 0, sizeof(*c));
	set_idle(IDLE_SETUP_S);
	entry(c);
}


static void ui_new_resume(void *ctx)
{
	struct ui_new_ctx *c = ctx;
	uint32_t pin;

	switch (c->stage) {
	case S_NEW:
		if (!*c->buf) {
			turn_off();
			return;
		}
		c->new_pin = pin_encode(c->buf);
		break;
	case S_CONFIRM:
		if (!*c->buf) {
			ui_new_open(c, NULL);
			return;
		}
		pin = pin_encode(c->buf);
		if (pin != c->new_pin) {
			notice_switch(&ui_new, NULL, nt_error, "PIN mismatch");
		} else if (pin_set(pin)) {
			write_new_settings();
		} else {
			notice_switch(&ui_off, NULL, nt_fault,
			    "Could not set PIN");
		}
		return;
	default:
		ABORT();
	}
	c->stage++;
	entry(c);
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_new = {
	.name		= "New device",
	.ctx_size	= sizeof(struct ui_new_ctx),
	.open		= ui_new_open,
	.resume		= ui_new_resume,
};
