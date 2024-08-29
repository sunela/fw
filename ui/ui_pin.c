/*
 * ui_pin.c - User interface: PIN input
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "hal.h"
#include "debug.h"
#include "gfx.h"
#include "db.h"
#include "ui_accounts.h"
#include "pin.h"
#include "ui_entry.h"
#include "wi_pin_entry.h"
#include "ui.h"


/* --- Database opening progress ------------------------------------------- */

#define	PROGRESS_TOTAL_COLOR	GFX_HEX(0x202020)
#define	PROGRESS_DONE_COLOR	GFX_GREEN

#define	PROGRESS_W		180
#define	PROGRESS_H		20
#define	PROGRESS_X0		((GFX_WIDTH - PROGRESS_W) / 2)
#define	PROGRESS_Y0		((GFX_HEIGHT - PROGRESS_H) / 2)


struct ui_pin_ctx {
	char buf[MAX_PIN_LEN + 1];	
	struct wi_pin_entry_ctx pin_entry_ctx;
};


/* --- Event handling ------------------------------------------------------ */


static void open_progress(void *user, unsigned i, unsigned n)
{
	unsigned *progress = user;
	unsigned w = n ? i * PROGRESS_W / n : 0;

	if (*progress == w)
		return;
	gfx_rect_xy(&main_da, PROGRESS_X0 + *progress, PROGRESS_Y0,
	    w - *progress, PROGRESS_H, PROGRESS_DONE_COLOR);
	ui_update_display();
	*progress = w;
}


static bool accept_pin(uint32_t pin)
{
	struct db_stats s;
	unsigned progress = 0;

	if (!pin_validate(pin))
		return 0;
	ui_accounts_cancel_move();
	gfx_clear(&main_da, GFX_BLACK);
	gfx_rect_xy(&main_da, PROGRESS_X0, PROGRESS_Y0, PROGRESS_W, PROGRESS_H,
	    PROGRESS_TOTAL_COLOR);
	ui_update_display();	/* give immediate visual feedback */
	if (!db_open_progress(&main_db, NULL, open_progress, &progress))
		return 0;
	db_stats(&main_db, &s);
	/*
	 * @@@ also let us in if there Flash has been erased. This is for
	 * development - in the end, an erased Flash should bypass the PIN
	 * dialog and go through a setup procedure, e.g., asking for a new PIN,
	 * and writing some record (configuration ?), to "pin" the PIN.
	 */
	return s.data || s.empty || (s.erased == s.total);
}


/* --- Open/close ---------------------------------------------------------- */


static int validate(void *user, const char *s)
{
	unsigned len = strlen(s);

	return len >= MIN_PIN_LEN && len <= MAX_PIN_LEN;
}


static void ui_pin_open(void *ctx, void *params)
{
	struct ui_pin_ctx *c = ctx;
	struct ui_entry_params entry_params = {
		.input = {
			.buf		= c->buf,
			.max_len	= MAX_PIN_LEN,
			.validate	= validate,
		},
		.maps = &ui_entry_decimal_maps,
		.entry_ops = &wi_pin_entry_ops,
		.entry_user = &c->pin_entry_ctx,
        };

	wi_pin_entry_setup(&c->pin_entry_ctx, 1, pin_shuffle);
	memset(c->buf, 0, MAX_PIN_LEN + 1);
	set_idle(IDLE_PIN_S);
	ui_call(&ui_entry, &entry_params);
}


static void ui_pin_resume(void *ctx)
{
	struct ui_pin_ctx *c = ctx;
	uint32_t pin;

	if (!*c->buf) {
		memset(c, 0, sizeof(*c));
		turn_off();
		return;
	}

	pin = pin_encode(c->buf);
	progress();
	if (accept_pin(pin)) {
		pin_attempts = 0;
		pin_cooldown = 0;
		ui_switch(&ui_accounts, NULL);
	} else {
		ui_switch(&ui_fail, NULL);
	}
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_pin = {
	.name		= "pin",
	.ctx_size	= sizeof(struct ui_pin_ctx),
	.open		= ui_pin_open,
	.resume		= ui_pin_resume,
};
