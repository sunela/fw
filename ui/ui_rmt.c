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
#include "timer.h"
#include "fmt.h"
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
#define	TIME_FONT	mono18
#define	SUBTEXT_FONT	mono18

#define	TEXT_Y0		(GFX_HEIGHT / 2 - 10)
#define	SUBTEXT_Y0	(GFX_HEIGHT / 2 + 60)

#define	TIMEOUT_Y0	35

#define	TIME_FG		GFX_BLACK
#define	TIME_BG		GFX_HEX(0x808080)

#define	TIME_BOX_R	4

#define	QUESTION_Y0	80
#define	QUESTION_H	120
#define QUESTION_FONT	mono18

#define	YESNO_Y0	220
#define	YESNO_H		50
#define	YESNO_W		90
#define	YESNO_R		5
#define	YESNO_FONT	mono18
#define	YESNO_NO_BG	GFX_RED
#define	YESNO_YES_BG	GFX_GREEN
#define	YESNO_FG	GFX_BLACK


struct ui_rmt_ctx {
	bool revealing;
	uint64_t timeout_ms;
	struct gfx_rect cd_box;
	unsigned last_s;
	/* action for which permission is being requested; NULL if none */
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
	text_query(0, 0, "88h88", &TIME_FONT, GFX_LEFT, GFX_TOP, &q);
	x = GFX_WIDTH - q.w - 2 * TIME_BOX_R;
	gfx_rrect_xy(&main_da, x, TIMEOUT_Y0,
	    q.w + 2 * TIME_BOX_R, q.h + 2 * TIME_BOX_R, TIME_BOX_R, TIME_BG);
	c->cd_box.x = x;
	c->cd_box.y = TIMEOUT_Y0;
	c->cd_box.w = q.w + 2 * TIME_BOX_R;
	c->cd_box.h = q.h + 2 * TIME_BOX_R;

	if (s >= 3600 * 100)
		return;
	if (s >= 60 * 100)
		format(add_char, &p, "%2uh%02u", s / 3600, (s / 60) % 60);
	else
		format(add_char, &p, "%2u:%02u", s / 60, s % 60);
	text_text(&main_da, x + TIME_BOX_R + q.ox,
	    TIMEOUT_Y0 + TIME_BOX_R + q.oy,
	    buf, &TIME_FONT, GFX_ORIGIN, GFX_ORIGIN, TIME_FG);
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


static void action_reveal(struct ui_rmt_ctx *c, void *user)
{
	const struct db_field *f = user;
	char buf[f->len + 1];

	if (main_db.generation != c->generation) {
		/* @@@ should display some message */
		show_remote();
		ui_update_display();
		return;
	}

	memcpy(buf, f->data, f->len);
	buf[f->len] = 0;

	gfx_clear(&main_da, GFX_BLACK);
	text_text(&main_da, GFX_WIDTH / 2, GFX_HEIGHT / 2, buf,
	    &TEXT_FONT, GFX_CENTER, GFX_CENTER, GFX_YELLOW);
	last_ctx->revealing = 1;
	ui_update_display();
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

		/* Yes */
		yesno_rect(&r, 1);
		if (gfx_in_rect(&r, x, y)) {
			void (*action)(struct ui_rmt_ctx *c, void *user) =
			    c->action;
			void *user = c->user;

			c->action = NULL;
			c->user = NULL;
			action(c, user);
			return;
		}

		/* No */
		yesno_rect(&r, 0);
		if (!gfx_in_rect(&r, x, y))
			return;
	}
	if (c->revealing || c->action) {
		show_remote();
		c->revealing = 0;
		c->action = 0;
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
	const char *field_name = "password";

	/* last_ctx is not set if we're running from a test script */
	if (scripting && !last_ctx)
		return;
	last_ctx->generation = main_db.generation;
	ask_permission(last_ctx, action_reveal, (void *) field->f,
	    "Show %s of %s ?", field_name, field->de->name);
}
