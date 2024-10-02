/*
 * ui_show_master.c - User interface: show the master secret
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <string.h>

#include "fmt.h"
#include "pin.h"
#include "gfx.h"
#include "bip39enc.h"
#include "secrets.h"
#include "wi_list.h"
#include "style.h"
#include "ui.h"
#include "wi_pin_entry.h"
#include "ui_notice.h"
#include "ui_entry.h"


static void ui_show_master_2_open(void *ctx, void *params);


struct ui_show_master_ctx {
	struct wi_list list;
	char buf[MAX_PIN_LEN + 1];
	struct wi_pin_entry_ctx pin_entry_ctx;
};

static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.entry = {
		.fg	= { GFX_WHITE, GFX_WHITE },
		.bg	= { GFX_BLACK, GFX_HEX(0x202020) },
		.min_h	= 36,
	}
};


static struct wi_list *lists[1];

static const struct ui_events ui_show_master_2_events = {
	.touch_to       = swipe_back,
	.lists		= lists,
	.n_lists	= 1,
};

static const struct ui ui_show_master_2 = {
	.name		= "show master 2",
	.ctx_size	= sizeof(struct ui_show_master_ctx),
	.open		= ui_show_master_2_open,
	.events		= &ui_show_master_2_events,
};



/* --- Helper functions ---------------------------------------------------- */


static int validate(void *user, const char *s)
{
	unsigned len = strlen(s);

	return len >= MIN_PIN_LEN && len <= MAX_PIN_LEN;
}


/* --- Entry --------------------------------------------------------------- */


static void entry(struct ui_show_master_ctx *c)
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

	memset(c->buf, 0, MAX_PIN_LEN + 1);
	wi_pin_entry_setup(&c->pin_entry_ctx, 0, NULL);
	progress();
	ui_call(&ui_entry, &params);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_show_master_open(void *ctx, void *params)
{
	struct ui_show_master_ctx *c = ctx;

	memset(c, 0, sizeof(*c));
	entry(c);
}


static void ui_show_master_2_open(void *ctx, void *params)
{
	struct ui_show_master_ctx *c = ctx;
	struct bip39enc bip;
	unsigned n = 0;

	lists[0] = &c->list;

	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
        text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, "Master secret",
            &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&c->list, &style);
	bip39_encode(&bip, master_secret, sizeof(master_secret));
	while (1) {
		int i;

		i = bip39_next_word(&bip);
		if (i < 0)
			break;

		char buf[11];
		char *p = buf;

		n++;
		format(add_char, &p, "%2u ", n);
		/* @@@ fix format() */
		if (n < 10)
			buf[0] = ' ';
		strcpy(buf + 3, bip39_words[i]);
		wi_list_add(&c->list, buf, NULL, NULL);
	}
	wi_list_end(&c->list);
	set_idle(IDLE_MASTER_S);
}


static void ui_show_master_resume(void *ctx)
{
	struct ui_show_master_ctx *c = ctx;
	uint32_t pin;

	if (!*c->buf) {
		ui_return();
		return;
	}
	pin = pin_encode(c->buf);
	if (!pin_revalidate(pin)) {
		notice(nt_error, "Incorrect PIN");
		return;
	}
	ui_switch(&ui_show_master_2, NULL);
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_show_master = {
	.name		= "show master",
	.ctx_size	= sizeof(struct ui_show_master_ctx),
	.open		= ui_show_master_open,
	.resume		= ui_show_master_resume,
};
