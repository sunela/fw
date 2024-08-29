/*
 * ui_pin_change.c - User interface: Version information
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pin.h"
#include "gfx.h"
#include "ui.h"
#include "wi_pin_entry.h"
#include "wi_general_entry.h"
#include "ui_notice.h"
#include "ui_entry.h"


#define	FG		GFX_WHITE
#define	ERROR_BG	GFX_HEX(0x800000)
#define	SUCCESS_BG	GFX_HEX(0x008000)
#define	NEUTRAL_BG	GFX_HEX(0x202020)
#define	FAULT_BG	GFX_HEX(0x606000)


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
		abort();
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


enum notice_type {
	nt_success,
	nt_error,
	nt_neutral,
	nt_fault,
};


static void notice(struct ui_pin_change_ctx *c, const char *s,
    enum notice_type type)
{
	struct ui_notice_style style = {
		.fg		= FG,
		/* .bg is set below */
		.x_align	= GFX_CENTER,
	};
	struct ui_notice_params params = {
		.style		= &style,
		.s		= s,
	};

	switch (type) {
	case nt_success:
		style.bg = SUCCESS_BG;
		break;
	case nt_error:
		style.bg = ERROR_BG;
		break;
	case nt_fault:
		style.bg = FAULT_BG;
		break;
	case nt_neutral:
		style.bg = NEUTRAL_BG;
		break;
	default:
		abort();
	}
	ui_switch(&ui_notice, &params);
}


static void ui_pin_change_resume(void *ctx)
{
	struct ui_pin_change_ctx *c = ctx;
	uint32_t pin;

	if (!*c->buf) {
		notice(c, "PIN not changed", nt_neutral);
		return;
	}
	switch (c->stage) {
	case S_OLD:
		c->old_pin = pin_encode(c->buf);
		if (pin_revalidate(c->old_pin))
			break;
		notice(c, "Incorrect PIN", nt_error);
		return;
	case S_NEW:
		c->new_pin = pin_encode(c->buf);
		if (!pin_revalidate(c->new_pin))
			break;
		notice(c, "Same PIN", nt_error);
		return;
	case S_CONFIRM:
		pin = pin_encode(c->buf);
		if (pin != c->new_pin) {
			notice(c, "PIN mismatch", nt_error);
			return;
		}
		switch (pin_change(c->old_pin, pin)) {
		case 1:
			notice(c, "PIN changed", nt_success);
			break;
		case 0:
			notice(c, "Change refused", nt_error);
			break;
		case -1:
			notice(c, "Change failed", nt_fault);
			break;
		default:
			abort();
		}
		return;
	default:
		abort();
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
