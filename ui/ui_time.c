/*
 * ui_time.c - User interface: set the time
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ To do:
 * - also support time setting from BTLE
 * - we should only silently accept time changes that don't deviate from a
 *   long-term projection by more than ~20 ppm. For larger changes, ask for
 *   confirmation.
 * - needs a way to enter time and date manually
 * - needs a way to set the time zone. Should also take it from USB.
 * - needs a way to exit
 * - the Manual/USB entry is difficult to tap
 */

#include <stddef.h>
#include <time.h>
#include <sys/types.h>

#include "hal.h"
#include "debug.h"
#include "fmt.h"
#include "gfx.h"
#include "text.h"
#include "wi_list.h"
#include "style.h"
#include "ui.h"


struct ui_time_ctx {
	struct wi_list list;
	struct wi_list_entry *entry_time, *entry_date, *entry_sync;
};


static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.opad	= 3,
	.entry = {
		.fg	= { GFX_WHITE, GFX_WHITE },
		.bg	= { GFX_BLACK, GFX_HEX(0x202020) },
		.min_h	= 50,
	}
};

static struct wi_list *lists[1];


/* --- Show time ----------------------------------------------------------- */


static void show_time(struct ui_time_ctx *c)
{
	static char s_time[9];
	static char s_date[11];
	char *p_time = s_time;
	char *p_date = s_date;
	struct tm tm;
	time_t t;

	t = time_us() / 1000000 + time_offset;
	gmtime_r(&t, &tm);
	format(add_char, &p_time, "%02d:%02d:%02d",
	    tm.tm_hour, tm.tm_min, tm.tm_sec);
	format(add_char, &p_date, "%04d-%02d-%02d",
	    tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
	wi_list_update_entry(&c->list, c->entry_time, "Time", s_time, NULL);
	wi_list_update_entry(&c->list, c->entry_date, "Date", s_date, NULL);
}


/* --- Event handling ------------------------------------------------------ */


static void ui_time_tap(void *ctx, unsigned x, unsigned y)
{
#if 0
	struct ui_time_ctx *c = ctx;
	const struct wi_list_entry *entry;

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;
#endif
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_time_open(void *ctx, void *params)
{
	struct ui_time_ctx *c = ctx;

	lists[0] = &c->list;

	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, "Set Time",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&c->list, &style);
	c->entry_time = wi_list_add(&c->list, "Time", "--:--:--", NULL);
	c->entry_date = wi_list_add(&c->list, "Date", "****-**-**", NULL);
	wi_list_add(&c->list, "Time zone", "UTC", NULL);
	wi_list_end(&c->list);
	show_time(c);

	set_idle(IDLE_SET_TIME_S);
}


static void ui_time_close(void *ctx)
{
	struct ui_time_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


/* --- Timer ticks --------------------------------------------------------- */


static void ui_time_tick(void *ctx)
{
	struct ui_time_ctx *c = ctx;
	static int64_t last_tick = -1;
	int64_t this_tick = time_us() / 1000000;

	if (last_tick == this_tick)
		return;
	last_tick = this_tick;
	show_time(c);
	ui_update_display();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_time_events = {
	.touch_tap	= ui_time_tap,
	.touch_to	= swipe_back,
	.tick		= ui_time_tick,
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_time = {
	.name		= "time",
	.ctx_size	= sizeof(struct ui_time_ctx),
	.open		= ui_time_open,
	.close		= ui_time_close,
	.events		= &ui_time_events,
};
