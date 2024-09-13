/*
 * ui_pin_change.c - User interface: Version information
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <string.h>

#include "pin.h"
#include "gfx.h"
#include "ui.h"
#include "wi_pin_entry.h"
#include "wi_general_entry.h"
#include "ui_notice.h"
#include "ui_entry.h"


enum stage {
	S_OLD		= 0,
	S_NEW		= 1,
	S_CONFIRM	= 2,
};

struct ui_pin_change_ctx {
	enum stage stage;
	char buf[MAX_PIN_LEN + 1];
	struct wi_pin_entry_ctx pin_entry_ctx;
	struct wi_general_entry_ctx general_entry_ctx;
	uint32_t old_pin;
	uint32_t new_pin;
};


/* --- Helper functions ---------------------------------------------------- */


static int validate(void *user, const char *s)
{
	unsigned len = strlen(s);

	return len >= MIN_PIN_LEN && len <= MAX_PIN_LEN;
}


/* --- Entry --------------------------------------------------------------- */


static void entry(struct ui_pin_change_ctx *c)
{
	struct ui_entry_params params = {
		.input = {
			.buf		= c->buf,
			.max_len	= MAX_PIN_LEN,
			.validate	= validate,
		},
		.maps = &ui_entry_decimal_maps,
		.entry_ops = &wi_general_entry_ops,
		.entry_user = &c->general_entry_ctx,
	};

	memset(c->buf, 0, MAX_PIN_LEN + 1);
	switch (c->stage) {
	case S_OLD:
		params.input.title = "Current PIN";
		params.entry_ops = &wi_pin_entry_ops;
		params.entry_user = &c->pin_entry_ctx;
		wi_pin_entry_setup(&c->pin_entry_ctx, 0, NULL);
		break;
	case S_NEW:
		params.input.title = "New PIN";
		break;
	case S_CONFIRM:
		params.input.title = "Confirm PIN";
		break;
	default:
		ABORT();
	}
	progress();
	ui_call(&ui_entry, &params);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_pin_change_open(void *ctx, void *params)
{
	struct ui_pin_change_ctx *c = ctx;

	memset(c, 0, sizeof(*c));
	entry(c);
}


static void ui_pin_change_resume(void *ctx)
{
	struct ui_pin_change_ctx *c = ctx;
	uint32_t pin;

	if (!*c->buf) {
		notice(nt_info, "PIN not changed");
		return;
	}
	switch (c->stage) {
	case S_OLD:
		c->old_pin = pin_encode(c->buf);
		if (pin_revalidate(c->old_pin))
			break;
		notice(nt_error, "Incorrect PIN");
		if (pin_cooldown_ms())
			ui_switch(&ui_fail, NULL);
		return;
	case S_NEW:
		c->new_pin = pin_encode(c->buf);
		if (c->old_pin != c->new_pin)
			break;
		notice(nt_error, "Same PIN");
		return;
	case S_CONFIRM:
		pin = pin_encode(c->buf);
		if (pin != c->new_pin) {
			notice(nt_error, "PIN mismatch");
			return;
		}
		switch (pin_change(c->old_pin, pin)) {
		case 1:
			notice(nt_success, "PIN changed");
			break;
		case 0:
			notice(nt_error, "Change refused");
			break;
		case -1:
			notice(nt_fault, "Change failed");
			break;
		default:
			ABORT();
		}
		return;
	default:
		ABORT();
	}
	c->stage++;
	entry(c);
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_pin_change = {
	.name		= "PIN change",
	.ctx_size	= sizeof(struct ui_pin_change_ctx),
	.open		= ui_pin_change_open,
	.resume		= ui_pin_change_resume,
};
