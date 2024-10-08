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
#include "secrets.h"
#include "dbcrypt.h"
#include "db.h"
#include "style.h"
#include "ui.h"

#include "debug.h"


struct ui_storage_ctx {
	struct wi_list list;
	const struct wi_list_entry *initialize;
};

static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.entry = {
		.fg	= { GFX_WHITE, GFX_WHITE },
		.bg	= { GFX_BLACK, GFX_HEX(0x202020) },
		.min_h	= 50,
	}
};

static struct wi_list *lists[1];


static void initialize_storage(void)
{
	struct dbcrypt *c;
	unsigned total = storage_blocks();

	gfx_clear(&main_da, GFX_BLACK);
	ui_update_display();
	db_close(&main_db);
	storage_erase_blocks(0, total);
	// @@@ create at least one empty entry, so that we can verify the PIN
	secrets_init();
	c = dbcrypt_init(master_secret, sizeof(master_secret));
	db_open(&main_db, c);
}


/* --- Event handling ------------------------------------------------------ */


static void ui_storage_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_storage_ctx *c = ctx;
	const struct wi_list_entry *entry;

	entry = wi_list_pick(&c->list, x, y);
	if (entry && entry == c->initialize) {
		initialize_storage();
		ui_switch(&ui_storage, NULL);
	}
	return;
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_storage_open(void *ctx, void *params)
{
	struct ui_storage_ctx *c = ctx;
	struct db_stats s;
	char tmp[20];
	char *p;

	lists[0] = &c->list;

	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, "Storage",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	ui_update_display();

	db_stats(&main_db, &s);

	wi_list_begin(&c->list, &style);

	p = tmp;
	format(add_char, &p, "%u", s.total);
	wi_list_add(&c->list, "Total blocks", tmp, NULL);

	if (s.data) {
		p = tmp;
		format(add_char, &p, "%u", s.data);
		wi_list_add(&c->list, "Used", tmp, NULL);
	}

	if (s.special) {
		p = tmp;
		format(add_char, &p, "%u", s.special);
		wi_list_add(&c->list, "Special", tmp, NULL);
	}

	if (s.error) {
		p = tmp;
		format(add_char, &p, "%u", s.error);
		wi_list_add(&c->list, "Error", tmp, NULL);
	}

	if (s.erased) {
		p = tmp;
		format(add_char, &p, "%u", s.erased);
		wi_list_add(&c->list, "Erased", tmp, NULL);
	}

	if (s.deleted) {
		p = tmp;
		format(add_char, &p, "%u", s.deleted);
		wi_list_add(&c->list, "Deleted", tmp, NULL);
	}

	if (s.invalid) {
		p = tmp;
		format(add_char, &p, "%u", s.invalid);
		wi_list_add(&c->list, "Invalid", tmp, NULL);
	}

	if (s.empty) {
		p = tmp;
		format(add_char, &p, "%u", s.empty);
		wi_list_add(&c->list, "Empty", tmp, NULL);
	}

	/*
	 * "Invalid" includes blocks that were encrypted with a different key.
	 */
	if (s.data + s.invalid)
		c->initialize = NULL;
	else
		c->initialize = wi_list_add(&c->list, "Initialize", NULL, NULL);

	wi_list_end(&c->list);

	set_idle(IDLE_SETUP_S);
}


static void ui_storage_close(void *ctx)
{
	struct ui_storage_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_storage_events = {
	.touch_tap	= ui_storage_tap,
	.touch_to	= swipe_back,
	.lists		= lists,
	.n_lists	= 1,

};

const struct ui ui_storage = {
	.name		= "storage",
	.ctx_size	= sizeof(struct ui_storage_ctx),
	.open		= ui_storage_open,
	.close		= ui_storage_close,
	.events		= &ui_storage_events,
};
