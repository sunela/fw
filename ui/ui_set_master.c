/*
 * ui_set_master.c - User interface: Set the master secret
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "text.h"
#include "gfx.h"
#include "ui.h"
#include "bip39in.h"
#include "bip39dec.h"
#include "pin.h"
//#include "settings.h"
//#include "dbcrypt.h"
//#include "db.h"
#include "wi_pin_entry.h"
#include "wi_bip39_entry.h"
#include "ui_notice.h"
#include "ui_entry.h"
#include "ui_bip39.h"


#define	WORDS	24


enum stage {
	S_PIN		= 0,
	S_WORDS		= 1,
	S_VERIFY	= 2,
};

struct ui_set_master_ctx {
	enum stage stage;
	char buf[MAX_PIN_LEN + 1];
	uint16_t words[WORDS];
	unsigned num_words;
	uint8_t bytes[BIP39_MAX_BYTES];
	struct wi_pin_entry_ctx pin_entry_ctx;
	uint32_t pin;
};


/* --- Helper functions ---------------------------------------------------- */


static int validate(void *user, const char *s)
{
	unsigned len = strlen(s);

	return len >= MIN_PIN_LEN && len <= MAX_PIN_LEN;
}


/* --- Entry --------------------------------------------------------------- */


static void entry(struct ui_set_master_ctx *c)
{
	struct ui_entry_params params = {
		.input = {
			.title		= "Enter PIN",
			.buf		= c->buf,
			.max_len	= MAX_PIN_LEN,
			.validate	= validate,
		},
		.maps = &ui_entry_decimal_maps,
		.entry_ops = &wi_pin_entry_ops,
		.entry_user = &c->pin_entry_ctx,
	};

	memset(c->buf, 0, sizeof(c->buf));
	wi_pin_entry_setup(&c->pin_entry_ctx, 0, NULL);
	progress();
	ui_call(&ui_entry, &params);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_set_master_open(void *ctx, void *params)
{
	struct ui_set_master_ctx *c = ctx;

	memset(c, 0, sizeof(*c));
	set_idle(IDLE_SECRET_S);
	entry(c);
}


static void ui_set_master_resume(void *ctx)
{
	struct ui_set_master_ctx *c = ctx;

	if (!*c->buf) {
		ui_return();
		return;
	}

	switch (c->stage) {
	case S_PIN:
		c->pin = pin_encode(c->buf);
		if (!pin_revalidate(c->pin)) {
			notice(nt_error, "Incorrect PIN");
			if (pin_cooldown_ms())
				ui_switch(&ui_fail, NULL);
			return;
		}
		struct ui_bip39_params params = {
			.result		= c->words,
			.num_words	= &c->num_words,
			.max_words	= WORDS,
		};

		c->stage = S_WORDS;
		ui_call(&ui_bip39, &params);
		break;
	case S_WORDS:
		debug("words: %u\n", c->num_words);
		if (!c->num_words) {
			ui_return();
			return;
		}

		int n = bip39_decode(c->bytes, sizeof(c->bytes),
		    c->words, c->num_words);

		switch (n) {
		case BIP39_DECODE_UNRECOGNIZED:
			ABORT();
		case BIP39_DECODE_OVERFLOW:
			ABORT();
		case BIP39_DECODE_CHECKSUM:
			notice(nt_error, "Checksum error");
			return;
		default:
			assert(n == MASTER_SECRET_BYTES);
// should ask for confirmation
			memcpy(master_secret, c->bytes, n);
// update pad
// re-open db (or just power off ? - we are going for a major reset after all)
			notice(nt_success, "Master secret changed");
			return;
		}
#if 0
		unsigned i;

		debug("%u\n", c->num_words);
		for (i = 0; i != c->num_words; i++)
			debug("%u: %s\n", i + 1, bip39_words[c->words[i]]);
#endif
		break;
	default:
		ABORT();
	}

}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_set_master = {
	.name		= "set master",
	.ctx_size	= sizeof(struct ui_set_master_ctx),
	.open		= ui_set_master_open,
	.resume		= ui_set_master_resume,
};
