/*
 * ui_account.c - User interface: show account details
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "hal.h"
#include "fmt.h"
#include "base32.h"
#include "hotp.h"
#include "gfx.h"
#include "shape.h"
#include "text.h"
#include "accounts.h"
#include "wi_list.h"
#include "ui.h"


#define	FONT_TOP		mono18

#define	TITLE_FG		GFX_YELLOW
#define	TIMER_FG		GFX_HEX(0x8080ff)
#define	ENTRY_FG		GFX_WHITE
#define	EVEN_BG			GFX_BLACK
#define	ODD_BG			GFX_HEX(0x202020)

#define	TOP_H			30
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)


static void render_account(const struct wi_list *l,
    const struct wi_list_entry *entry, const struct gfx_rect *bb,
    bool odd);


static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.fg	= { ENTRY_FG, ENTRY_FG },
	.bg	= { EVEN_BG, ODD_BG },
	.min_h	= 40,
	.render	= render_account,
};

static struct account *selected_account = NULL;
static struct wi_list list;


/* --- Extra account rendering --------------------------------------------- */


static void render_account(const struct wi_list *l,
    const struct wi_list_entry *entry, const struct gfx_rect *bb,
    bool odd)
{
	const struct account *a = selected_account;
	unsigned passed_s = (time_us() / 1000000 + 1) % 30;

	if (!a->token.secret_size)
		return;
	if (a->token.type != tt_totp)
		return;
	gfx_arc(&da, bb->x + bb->w - 1 - bb->h / 2, bb->y + bb->h / 2,
	    bb->h / 4, 12 * passed_s, 0,
	    passed_s ? TIMER_FG : style.bg[odd], style.bg[odd]);
}


static void show_totp(struct wi_list *l,
    struct wi_list_entry *entry, void *user)
{
	struct account *a = wi_list_user(entry);

	if (!a)
		return;
	if (!a->token.secret_size)
		return;
	if (a->token.type != tt_totp)
		return;
	ssize_t size = base32_decode_size((char *) a->token.secret);

	assert(size > 0);

	uint8_t sec[size];
	uint64_t counter = (time_us() + time_offset) / 1000000 / 30;
	uint32_t code;
	char s[6 + 1];
	char *p = s;

	base32_decode(sec, size, (char *) a->token.secret);
	code = hotp64(sec, size, counter);
	format(add_char, &p, "%06u", (unsigned) code % 1000000);
	wi_list_update_entry(l, entry, "TOTP", s, a);
	wi_list_render(l, entry);
	update_display(&da);
}


static void ui_account_tick(void)
{
	static int64_t last_tick = -1;
	int64_t this_tick = time_us() / 1000000;

	if (last_tick == this_tick)
                return;
	last_tick = this_tick;
	/*
	 * @@@ we update every second for the "time left" display. The code
	 * changes only every 30 seconds.
	 */

	wi_list_forall(&list, show_totp, NULL);
}


/* --- Event handling ------------------------------------------------------ */


static void ui_account_tap(unsigned x, unsigned y)
{
	struct wi_list_entry *entry;
	struct account *a;
	char s[6 + 1];
	char *p = s;
	uint32_t code;

	entry = wi_list_pick(&list, x, y);
	if (!entry)
		return;
	a = wi_list_user(entry);
	if (!a)
		return;
	if (!a->token.secret_size || a->token.type != tt_hotp)
		return;

	progress();

	/* @@@ make it harder to update the counter ? */
	a->token.counter++;
	code = hotp64(a->token.secret, a->token.secret_size, a->token.counter);
	format(add_char, &p, "%06u", (unsigned) code % 1000000);
	wi_list_update_entry(&list, entry, "HOTP", s, a);
	update_display(&da);
}


static void ui_account_to(unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_account_open(void *params)
{
	struct account *a = selected_account = params;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, a->name, &FONT_TOP,
	    GFX_CENTER, GFX_CENTER, TITLE_FG);

	wi_list_begin(&list, &style);
	if (a->user)
		wi_list_add(&list, "User", a->user, NULL);
	if (a->pw)
		wi_list_add(&list, "Password", a->pw, NULL);
	if (a->token.secret_size) {
		switch (a->token.type) {
		case tt_hotp:
			wi_list_add(&list, "HOTP", "------", a);
			break;
		case tt_totp:
			wi_list_add(&list, "TOTP", "------", a);
			break;
		default:
			abort();
		}
	}
	wi_list_end(&list);

	set_idle(IDLE_ACCOUNT_S);
}


static void ui_account_close(void)
{
	wi_list_destroy(&list);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_account_events = {
	.touch_tap	= ui_account_tap,
	.touch_to	= ui_account_to,
	.tick		= ui_account_tick,
};

const struct ui ui_account = {
	.open = ui_account_open,
	.close = ui_account_close,
	.events = &ui_account_events,
};
