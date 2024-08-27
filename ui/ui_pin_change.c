/*
 * ui_pin_change.c - User interface: Version information
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "pin.h"
#include "gfx.h"
#include "ui.h"
#include "wi_general_entry.h"
#include "ui_notice.h"
#include "ui_entry.h"


#define	FG	GFX_WHITE
#define	FAIL_BG	GFX_HEX(0x800000)
#define	OK_BG	GFX_HEX(0x008000)


enum stage {
	S_OLD		= 0,
	S_NEW		= 1,
	S_CONFIRM	= 2,
};

struct ui_pin_change_ctx {
	enum stage stage;
	char buf[MAX_PIN_LEN + 1];
	struct wi_general_entry_ctx entry_ctx;
	uint32_t new_pin;
};


/* --- Helper functions ---------------------------------------------------- */


static uint32_t encode(const char *s)
{
	uint32_t pin = 0xffffffff;

	while (*s) {
		pin = pin << 4 | (*s - '0');
		s++;
	}
	return pin;
}


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
		.entry_user = &c->entry_ctx,
	};

	memset(c->buf, 0, MAX_PIN_LEN + 1);
	switch (c->stage) {
	case S_OLD:
		params.input.title = "Current PIN";
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


static void notice(struct ui_pin_change_ctx *c, const char *s, bool ok)
{
	struct ui_notice_style style = {
		.fg		= FG,
		.bg		= ok ? OK_BG : FAIL_BG,
		.x_align	= GFX_CENTER,
	};
	struct ui_notice_params params = {
		.style		= &style,
		.s		= s,
	};
	ui_switch(&ui_notice, &params);
}


static void ui_pin_change_resume(void *ctx)
{
	struct ui_pin_change_ctx *c = ctx;
	uint32_t pin;

	switch (c->stage) {
	case S_OLD:
		pin = encode(c->buf);
		if (pin == DUMMY_PIN)
			break;
		notice(c, "Incorrect PIN", 0);
		return;
	case S_NEW:
		c->new_pin = encode(c->buf);
		if (c->new_pin != DUMMY_PIN)
			break;
		notice(c, "Same PIN", 0);
		return;
	case S_CONFIRM:
		pin = encode(c->buf);
		if (pin == c->new_pin) {
			debug("NEW PIN 0x%08x\n", pin);
			/* store new PIN */
			notice(c, "PIN changed", 1);
		} else {
			notice(c, "PIN mismatch", 0);
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
