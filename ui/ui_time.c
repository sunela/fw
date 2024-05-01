/*
 * ui_time.c - User interface: set the time
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <time.h>

#include "hal.h"
#include "fmt.h"
#include "gfx.h"
#include "text.h"
#include "ui_list.h"
#include "ui.h"


#define	FONT_TOP		mono24

#define	TOP_H			30
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)

#define	HOLD_MS	(5 * 1000)


static const struct ui_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
};

enum sync_mode {
	sm_manual,
	sm_usb,
	sm_end		/* must be last */
};

static struct ui_list list;
static struct ui_list_entry *entry_time, *entry_date, *entry_sync;
static enum sync_mode sync_mode;

static const char *sync_mode_name[] = {
	"Manual",
	"USB"
};


/* --- Show time ----------------------------------------------------------- */


static uint64_t t_offset = 0;


static void show_time(void)
{
	static char s_time[9];
	static char s_date[11];
	char *p_time = s_time;
	char *p_date = s_date;
	struct tm tm;
	time_t t;

	t = time_us() / 1000000 + t_offset;
	gmtime_r(&t, &tm);
	format(add_char, &p_time, "%02d:%02d:%02d",
	    tm.tm_hour, tm.tm_min, tm.tm_sec);
	format(add_char, &p_date, "%04d-%02d-%02d",
	    tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
	ui_list_update_entry(&list, entry_time, "Time", s_time, NULL);
	ui_list_update_entry(&list, entry_date, "Date", s_date, NULL);
	ui_list_update_entry(&list, entry_sync, sync_mode_name[sync_mode],
	    NULL, NULL);
}


/* --- Sync mode ----------------------------------------------------------- */


static void show_sync_mode(void)
{
	ui_list_update_entry(&list, entry_sync, sync_mode_name[sync_mode],
	    NULL, NULL);
}


static void change_sync_mode(void)
{
	sync_mode = (sync_mode + 1) % sm_end;
	show_sync_mode();
	update_display(&da);
}


/* --- Event handling ------------------------------------------------------ */


static void ui_time_tap(unsigned x, unsigned y)
{
	const struct ui_list_entry *entry;

	entry = ui_list_pick(&list, x, y);
	if (!entry)
		return;
	if (entry == entry_sync)
		change_sync_mode();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_time_open(void)
{
	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Set Time",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	sync_mode = sm_manual;

	ui_list_begin(&list, &style);
	entry_time = ui_list_add(&list, "Time", "--:--:--", NULL);
	entry_date = ui_list_add(&list, "Date", "****-**-**", NULL);
	ui_list_add(&list, "Time Zone", "UTC", NULL);
	entry_sync = ui_list_add(&list, "", NULL, NULL);
	ui_list_end(&list);
	show_sync_mode();
}


static void ui_time_close(void)
{
	ui_list_destroy(&list);
}


/* --- Timer ticks --------------------------------------------------------- */


static void ui_time_tick(void)
{
	static int64_t last_tick = -1;
	int64_t this_tick = time_us() / 1000000;

	if (last_tick == this_tick)
		return;
	last_tick = this_tick;
	show_time();
	update_display(&da);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_time_events = {
	.touch_tap	= ui_time_tap,
	.tick		= ui_time_tick,
};

const struct ui ui_time = {
	.open = ui_time_open,
        .close = ui_time_close,
	.events = &ui_time_events,
};