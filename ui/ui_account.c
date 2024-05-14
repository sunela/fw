/*
 * ui_account.c - User interface: show account details
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "alloc.h"
#include "fmt.h"
#include "base32.h"
#include "hotp.h"
#include "gfx.h"
#include "shape.h"
#include "text.h"
#include "db.h"
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
    const struct wi_list_entry *entry, struct gfx_drawable *d,
    const struct gfx_rect *bb, bool odd);


static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.fg	= { ENTRY_FG, ENTRY_FG },
	.bg	= { EVEN_BG, ODD_BG },
	.min_h	= 40,
	.render	= render_account,
};

static struct db_entry *selected_account = NULL;
static struct wi_list list;
static struct wi_list *lists[1] = { &list };


/* --- Extra account rendering --------------------------------------------- */


static void render_account(const struct wi_list *l,
    const struct wi_list_entry *entry, struct gfx_drawable *d,
    const struct gfx_rect *bb, bool odd)
{
	const struct db_entry *de = selected_account;
	const struct db_field *f;
	unsigned passed_s = (time_us() / 1000000 + 1) % 30;

	for (f = de->fields; f; f = f->next)
		if (f->type == ft_totp_secret)
			break;
	if (!f)
		return;
	gfx_arc(d, bb->x + bb->w - 1 - bb->h / 2, bb->y + bb->h / 2,
	    bb->h / 4, 12 * passed_s, 0,
	    passed_s ? TIMER_FG : style.bg[odd], style.bg[odd]);
}


static void show_totp(struct wi_list *l,
    struct wi_list_entry *entry, void *user)
{
	struct db_entry *de = wi_list_user(entry);
	const struct db_field *f;

	if (!de)
		return;
	for (f = de->fields; f; f = f->next)
		if (f->type == ft_totp_secret)
			break;
	if (!f)
		return;

	assert(f->len > 0);

	uint64_t counter = (time_us() + time_offset) / 1000000 / 30;
	uint32_t code;
	char s[6 + 1];
	char *p = s;

	code = hotp64(f->data, f->len, counter);
	format(add_char, &p, "%06u", (unsigned) code % 1000000);
	wi_list_update_entry(l, entry, "TOTP", s, de);
	wi_list_render_entry(l, entry);
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
	struct db_entry *de;
	struct db_field *f;
	const struct db_field *hotp_secret = NULL;
	unsigned hotp_secret_len;
	struct db_field *hotp_counter = NULL;
	uint64_t counter;

	char s[6 + 1];
	char *p = s;
	uint32_t code;

	entry = wi_list_pick(&list, x, y);
	if (!entry)
		return;
	de = wi_list_user(entry);
	if (!de)
		return;
	for (f = de->fields; f; f = f->next)
		switch (f->type) {
		case ft_hotp_secret:
			hotp_secret = f->data;
			hotp_secret_len = f->len;
			break;
		case ft_hotp_counter:
			hotp_counter = f->data;
			assert(f->len == sizeof(counter));
			break;
		default:
			break;
		}
	if (!hotp_secret || !hotp_counter)
		return;

	progress();

	memcpy(&counter, hotp_counter, sizeof(counter));
	code = hotp64(hotp_secret, hotp_secret_len, counter);
	format(add_char, &p, "%06u", (unsigned) code % 1000000);
	wi_list_update_entry(&list, entry, "HOTP", s, de);
	update_display(&da);

	counter++;
	if (!db_change(de, ft_hotp_counter, &counter, sizeof(counter)))
		debug("HOTP counter increment failed\n");
}


static void ui_account_to(unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void add_string(const char *label, const char *s, unsigned len,
    void *user)
{
	char *tmp = alloc_size(len + 1);

	memcpy(tmp, s, len);
	tmp[len] = 0;
	wi_list_add(&list, label, tmp, user);
	free(tmp);
}


static void ui_account_open(void *params)
{
	struct db_entry *de = selected_account = params;
	const struct db_field *f;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, de->name, &FONT_TOP,
	    GFX_CENTER, GFX_CENTER, TITLE_FG);

	wi_list_begin(&list, &style);
	for (f = de->fields; f; f = f->next)
		switch (f->type) {
		case ft_id:
		case ft_prev:
			break;
		case ft_user:
			add_string("User", f->data, f->len, NULL);
			break;
		case ft_email:
			add_string("E-Mail", f->data, f->len, NULL);
			break;
		case ft_pw:
			add_string("Password", f->data, f->len, NULL);
			break;
		case ft_hotp_secret:
			wi_list_add(&list, "HOTP", "------", de);
			break;
		case ft_hotp_counter:
			break;
		case ft_totp_secret:
			wi_list_add(&list, "TOTP", "------", de);
			break;
		default:
			abort();
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
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_account = {
	.open = ui_account_open,
	.close = ui_account_close,
	.events = &ui_account_events,
};
