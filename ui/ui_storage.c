/*
 * ui_storage.c - User interface: Storage management
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "wi_list.h"
#include "fmt.h"
#include "storage.h"
#include "block.h"
#include "db.h"
#include "ui.h"

#include "debug.h"

#define	FONT_TOP		mono24

#define	TOP_H			31
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)


static const struct wi_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
	min_h:	40,
};

static struct wi_list list;
static struct wi_list *lists[1] = { &list };
static const struct wi_list_entry *initialize = NULL;


static void initialize_storage(void)
{
	unsigned total = storage_blocks();

	gfx_clear(&da, GFX_BLACK);
	update_display(&da);
	db_close(&main_db);
	storage_erase_blocks(0, total);
	// @@@ create at least one empty entry, so that we can verify the PIN
	db_open(&main_db, NULL);
}


/* --- Event handling ------------------------------------------------------ */


static void ui_storage_tap(unsigned x, unsigned y)
{
	const struct wi_list_entry *entry;

	entry = wi_list_pick(&list, x, y);
	if (entry && entry == initialize) {
		initialize_storage();
		ui_switch(&ui_storage, NULL);
	}
	return;
}


static void ui_storage_to(unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_storage_open(void *params)
{
	struct db_stats s;
	char tmp[20];
	char *p;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Storage",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	update_display(&da);

	db_stats(&main_db, &s);

	wi_list_begin(&list, &style);

	p = tmp;
	format(add_char, &p, "%u", s.total);
	wi_list_add(&list, "Total blocks", tmp, NULL);

	p = tmp;
	format(add_char, &p, "%u", s.data);
	wi_list_add(&list, "Used", tmp, NULL);

	p = tmp;
	format(add_char, &p, "%u", s.error);
	wi_list_add(&list, "Error", tmp, NULL);

	p = tmp;
	format(add_char, &p, "%u", s.erased);
	wi_list_add(&list, "Erased", tmp, NULL);

	p = tmp;
	format(add_char, &p, "%u", s.deleted);
	wi_list_add(&list, "Deleted", tmp, NULL);

	p = tmp;
	format(add_char, &p, "%u", s.invalid);
	wi_list_add(&list, "Invalid", tmp, NULL);

	p = tmp;
	format(add_char, &p, "%u", s.empty);
	wi_list_add(&list, "Empty", tmp, NULL);

	/*
	 * "Invalid" includes blocks that were encrypted with a different key.
	 */
	if (s.data + s.invalid)
		initialize = NULL;
	else
		initialize = wi_list_add(&list, "Initialize", NULL, NULL);

	wi_list_end(&list);

	set_idle(IDLE_SETUP_S);
}


static void ui_storage_close(void)
{
	wi_list_destroy(&list);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_storage_events = {
	.touch_tap	= ui_storage_tap,
	.touch_to	= ui_storage_to,
	.lists		= lists,
	.n_lists	= 1,

};

const struct ui ui_storage = {
	.open = ui_storage_open,
	.close = ui_storage_close,
	.events = &ui_storage_events,
};
