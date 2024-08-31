/*
 * ui_rmt.c - User interface: Remote control
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hal.h"
#include "db.h"
#include "gfx.h"
#include "text.h"
#include "rmt.h"
#include "rmt-db.h"
#include "ui.h"
#include "ui_rmt.h"


#define	FONT	mono24


struct ui_rmt_ctx {
	bool revealing;
};

static void show_remote(void)
{
	gfx_clear(&main_da, GFX_BLACK);
	text_text(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2, "Remote",
	    &FONT, GFX_CENTER, GFX_CENTER, GFX_WHITE);

}


static struct ui_rmt_ctx *last_ctx;


/* --- Events -------------------------------------------------------------- */


static void ui_rmt_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_rmt_ctx *c = ctx;

	if (c->revealing) {
		show_remote();
		c->revealing = 0;
		ui_update_display();
	} else {
		ui_return();
	}
}


static void ui_rmt_tick(void *ctx)
{
	rmt_db_poll();
}


static void ui_rmt_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_rmt_open(void *ctx, void *params)
{
	struct ui_rmt_ctx *c = ctx;

	last_ctx = c;
	c->revealing = 0;
	show_remote();
	rmt_db_init();
	set_idle(3600); /* @@@ */
}


static void ui_rmt_close(void *ctx)
{
	rmt_close(&rmt_usb);
	last_ctx = NULL;
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_rmt_events = {
	.touch_tap	= ui_rmt_tap,
	.touch_to	= ui_rmt_to,
	.tick		= ui_rmt_tick,
};

const struct ui ui_rmt = {
	.name		= "rmt",
	.ctx_size	= sizeof(struct ui_rmt_ctx),
	.open		= ui_rmt_open,
	.close		= ui_rmt_close,
	.events		= &ui_rmt_events,
};


/* --- Interface towards the protocol stack -------------------------------- */


void ui_rmt_reveal(const struct db_field *f)
{
	char buf[f->len + 1];

	memcpy(buf, f->data, f->len);
	buf[f->len] = 0;

	gfx_clear(&main_da, GFX_BLACK);
	text_text(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2, buf,
	    &FONT, GFX_CENTER, GFX_CENTER, GFX_YELLOW);
	last_ctx->revealing = 1;
	ui_update_display();
}
