/*
 * ui_rmt.c - User interface: Remote control
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "alloc.h"
#include "debug.h"
#include "timer.h"
#include "fmt.h"
#include "base32.h"
#include "db.h"
#include "gfx.h"
#include "shape.h"
#include "text.h"
#include "rmt.h"
#include "rmt-db.h"
#include "ui.h"
#include "ui_rmt.h"


#define	TIMEOUT_S	1800

#define	TEXT_FONT	mono24
#define	TEXT_Y0		(GFX_HEIGHT / 2 - 10)

#define	SUBTEXT_FONT	mono18
#define	SUBTEXT_Y0	(GFX_HEIGHT / 2 + 60)

#define	TIMEOUT_X1	(GFX_WIDTH - 15)
#define	TIMEOUT_Y0	15
#define	TIMEOUT_FONT	mono18
#define	TIMEOUT_FG	GFX_BLACK
#define	TIMEOUT_BG	GFX_HEX(0x808080)
#define	TIMEOUT_BOX_R	4

#define	QUESTION_Y0	80
#define	QUESTION_H	120
#define QUESTION_FONT	mono18

#define	YESNO_Y0	220
#define	YESNO_H		50
#define	YESNO_W		90
#define	YESNO_R		10
#define	YESNO_FONT	mono18
#define	YESNO_NO_BG	GFX_RED
#define	YESNO_YES_BG	GFX_GREEN
#define	YESNO_FG	GFX_BLACK


struct ui_rmt_ctx {
	bool revealing;
	uint64_t timeout_ms;
	struct gfx_rect cd_box;
	unsigned last_s;
	/*
	 * action for which permission is being requested; NULL if none.
	 * "action" is invoked if the permission is granted, with "c" set to
	 * the context, and also if the permission is denied, with "c" set to
	 * NULL, in order to deallocate "user", if necessary.
	 */
	void (*action)(struct ui_rmt_ctx *c, void *user);
	void *user;
	unsigned generation; /* database generation */
};


static struct ui_rmt_ctx *last_ctx;


/* --- Show things --------------------------------------------------------- */


static void show_countdown(struct ui_rmt_ctx *c, unsigned s)
{
	char buf[6];
	char *p = buf;
	struct text_query q;
	unsigned x;

	/*
	 * We run this over the set of characters we actually use to avoid the
	 * addition of unused space we'd get with GFX_MAX.
	 */
	text_query(0, 0, "88h88", &TIMEOUT_FONT, GFX_LEFT, GFX_TOP, &q);
	x = TIMEOUT_X1 - q.w - 2 * TIMEOUT_BOX_R;
	gfx_rrect_xy(&main_da, x, TIMEOUT_Y0,
	    q.w + 2 * TIMEOUT_BOX_R, q.h + 2 * TIMEOUT_BOX_R, TIMEOUT_BOX_R,
	    TIMEOUT_BG);
	c->cd_box.x = x;
	c->cd_box.y = TIMEOUT_Y0;
	c->cd_box.w = q.w + 2 * TIMEOUT_BOX_R;
	c->cd_box.h = q.h + 2 * TIMEOUT_BOX_R;

	if (s >= 3600 * 100)
		return;
	if (s >= 60 * 100)
		format(add_char, &p, "%2uh%02u", s / 3600, (s / 60) % 60);
	else
		format(add_char, &p, "%2u:%02u", s / 60, s % 60);
	text_text(&main_da, x + TIMEOUT_BOX_R + q.ox,
	    TIMEOUT_Y0 + TIMEOUT_BOX_R + q.oy,
	    buf, &TIMEOUT_FONT, GFX_ORIGIN, GFX_ORIGIN, TIMEOUT_FG);
	ui_update_display();
}


static void show_remote(void)
{
	gfx_clear(&main_da, GFX_BLACK);
	text_text(&main_da, GFX_WIDTH / 2, TEXT_Y0, "Remote",
	    &TEXT_FONT, GFX_CENTER, GFX_CENTER, GFX_WHITE);
	text_format(&main_da, 0, SUBTEXT_Y0,
	    GFX_WIDTH, GFX_HEIGHT - SUBTEXT_Y0, 0,
	    "Swipe left to close", &SUBTEXT_FONT, GFX_CENTER, GFX_WHITE);
}


static void reset_timeout(struct ui_rmt_ctx *c)
{
	c->timeout_ms = now + TIMEOUT_S * 1000;
	set_idle(TIMEOUT_S);
}


/* --- Permission and actions ---------------------------------------------- */


static void do_action_reveal(struct ui_rmt_ctx *c, const char *s)
{
	unsigned h, y;

	if (main_db.generation != c->generation) {
		/* @@@ should display some message */
		show_remote();
		ui_update_display();
		return;
	}

	gfx_clear(&main_da, GFX_BLACK);
	h = text_format(&main_da, 0, 0, GFX_WIDTH, 0, 0, s, 
	    &TEXT_FONT, GFX_CENTER, GFX_YELLOW);
	if (h > GFX_HEIGHT)
		h = GFX_HEIGHT;
	y = (GFX_HEIGHT - h) / 2;
	text_format(&main_da, 0, y, GFX_WIDTH, GFX_HEIGHT - y, 0, s, 
	    &TEXT_FONT, GFX_CENTER, GFX_YELLOW);
	last_ctx->revealing = 1;
	ui_update_display();
}


static void action_reveal(struct ui_rmt_ctx *c, void *user)
{
	if (c)
		do_action_reveal(c, user);
	free(user);
}


static void yesno_rect(struct gfx_rect *r, bool right)
{
	unsigned gap = (GFX_WIDTH - 2 * YESNO_W) / 3;

	r->x = gap + (YESNO_W + gap) * right;
	r->y = YESNO_Y0;
	r->h = YESNO_H;
	r->w = YESNO_W;
}


static void yesno_button(struct ui_rmt_ctx *c, bool yes)
{
	struct gfx_rect r;

	yesno_rect(&r, yes);
	gfx_rrect(&main_da, &r, YESNO_R, yes ? YESNO_YES_BG : YESNO_NO_BG);
	text_text(&main_da, r.x + r.w / 2, r.y + r.h / 2, yes ? "Yes" : "No",
	    &YESNO_FONT, GFX_CENTER, GFX_CENTER, YESNO_FG);
}


static void ask_permission(struct ui_rmt_ctx *c,
    void (*fn)(struct ui_rmt_ctx *c, void *user), void *user,
    const char *fmt, ...)
{
	va_list ap;
	char *s;

	c->action = fn;
	c->user = user;
	va_start(ap, fmt);
	s = vformat_alloc(fmt, ap);
	va_end(ap);

	gfx_clear(&main_da, GFX_BLACK);
	text_format(&main_da, 0, QUESTION_Y0, GFX_WIDTH, QUESTION_H, 0,
	    s, &QUESTION_FONT, GFX_CENTER, GFX_WHITE);
	yesno_button(c, 0);
	yesno_button(c, 1);
	free(s);
	ui_update_display();
}


/* --- Events -------------------------------------------------------------- */


static void ui_rmt_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_rmt_ctx *c = ctx;

	if (c->action) {
		struct gfx_rect r;
		bool yes, no;

		yesno_rect(&r, 1);
		yes = gfx_in_rect(&r, x, y);
		yesno_rect(&r, 0);
		no = gfx_in_rect(&r, x, y);

		if (!yes && !no)
			return;

		void (*action)(struct ui_rmt_ctx *c, void *user) = c->action;
		void *user = c->user;

		c->action = NULL;
		c->user = NULL;
		c->action = NULL;

		/* Yes */
		if (yes) {
			action(c, user);
			return;
		}

		/* No */
		show_remote();
		action(NULL, user);
		return;
	}
	if (c->revealing || c->action) {
		show_remote();
		c->revealing = 0;
		c->action = NULL;
		ui_update_display();
	} else {
		if (gfx_in_rect(&c->cd_box, x,y))
			reset_timeout(c);
	}
}


static void ui_rmt_tick(void *ctx)
{
	struct ui_rmt_ctx *c = ctx;
	unsigned s = (c->timeout_ms - now + 999) / 1000;

	rmt_db_poll();
	if (c->last_s != s) {
		show_countdown(c, s);
		c->last_s = s;
	}
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
	c->last_s = 0;
	c->action = NULL;
	reset_timeout(c);

	show_remote();
	show_countdown(c, TIMEOUT_S);
	rmt_db_init();
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


void ui_rmt_reveal(const struct ui_rmt_field *field)
{
	const char *field_name;
	const struct db_field *f = field->f;
	char *buf;

	/* last_ctx is not set if we're running from a test script */
	if (scripting && !last_ctx)
		return;
	last_ctx->generation = main_db.generation;
	switch (f->type) {
	case ft_pw:
		field_name = "password";
		break;
	case ft_pw2:
		field_name = "2nd password";
		break;
	case ft_hotp_secret:
		field_name = "HOTP secret";
		break;
	case ft_totp_secret:
		field_name = "TOTP secret";
		break;
	default:
		debug("ui_rmt_reveal: unrecognized field %u\n", f->type);
		return;
	}
	if (field->binary) {
		size_t size = base32_encode_size(f->len);

		buf = alloc_size(size);
		base32_encode(buf, size, f->data, f->len);
	} else {
		buf = alloc_size(f->len + 1);
		memcpy(buf, f->data, f->len);
		buf[f->len] = 0;
	}
	ask_permission(last_ctx, action_reveal, buf,
	    "Show %s of %s ?", field_name, field->de->name);
}
