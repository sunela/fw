/*
 * ui_rd.c - User interface: R&D
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
//#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "util.h"
#include "gfx.h"
#include "shape.h"
#include "text.h"
#include "settings.h"
#include "wi_list.h"
#include "ui_overlay.h"
#include "style.h"
#include "ui.h"


#define	TITLE_FG		GFX_YELLOW
#define	ENTRY_FG		GFX_WHITE
#define	EVEN_BG			GFX_BLACK
#define	ODD_BG			GFX_HEX(0x202020)


enum rd_item_type {
	rit_bool,
};

struct rd_item {
	const char *label;
	enum rd_item_type type;
	union {
		bool *bool_var;
	} u;
};

struct ui_rd_ctx {
	struct wi_list list;
};

static void render_rd(const struct wi_list *l,
    const struct wi_list_entry *entry, struct gfx_drawable *d,
    const struct gfx_rect *bb, bool odd);


static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.fg	= { ENTRY_FG, ENTRY_FG },
	.bg	= { EVEN_BG, ODD_BG },
	.min_h	= 50,
	.render	= render_rd,
};

static struct wi_list *lists[1];


/* --- Rendering the setting  ---------------------------------------------- */


static void render_rd(const struct wi_list *l,
    const struct wi_list_entry *entry, struct gfx_drawable *d,
    const struct gfx_rect *bb, bool odd)
{
	const struct rd_item *item = wi_list_user(entry);

	switch (item->type) {
	case rit_bool:
		gfx_checkbox(d, bb->x + bb->w - 1 - bb->h / 2,
		    bb->y + bb->h / 2, bb->h / 2, 2, *item->u.bool_var,
		    ENTRY_FG, odd ? ODD_BG : EVEN_BG);
		break;
	default:
		abort();
	}
}


/* --- Tap ----------------------------------------------------------------- */


static void ui_rd_tap(void *ctx, unsigned x, unsigned y)
{
	struct ui_rd_ctx *c = ctx;
	struct wi_list_entry *entry;
	const struct rd_item *item;

	entry = wi_list_pick(&c->list, x, y);
	if (!entry)
		return;
 	item = wi_list_user(entry);

	switch (item->type) {
	case rit_bool:
		*item->u.bool_var = !*item->u.bool_var;
		wi_list_render_entry(&c->list, entry);
		break;
	default:
		abort();
	}

	progress();
	ui_update_display();
}


/* --- Long press ---------------------------------------------------------- */


static void power_off(void *user)
{
	turn_off();
}


static void rd_overlay(struct ui_rd_ctx *c)
{
	static struct ui_overlay_button buttons[] = {
		{ ui_overlay_sym_power,	power_off, NULL },
	};
	static struct ui_overlay_params prm = {
		.buttons	= buttons,
		.n_buttons	= 1,
	};
	unsigned i;

	for (i = 0; i != prm.n_buttons; i++)
		buttons[i].user = c;
	ui_call(&ui_overlay, &prm);
}


static void ui_rd_long(void *ctx, unsigned x, unsigned y)
{
	struct ui_rd_ctx *c = ctx;

	rd_overlay(c);
}


/* --- Swipe --------------------------------------------------------------- */


static void ui_rd_to(void *ctx, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_rd_open(void *ctx, void *params)
{
	static const struct rd_item items[] = {
		{ "Crosshair",	rit_bool,
		    { .bool_var = &settings.crosshair }},
	};
	struct ui_rd_ctx *c = ctx;
	const struct rd_item *item;

	lists[0] = &c->list;

	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, "R&D", &FONT_TOP,
	    GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&c->list, &style);
	for (item = items; item != items + ARRAY_ENTRIES(items); item++)
		wi_list_add(&c->list, item->label, NULL, (void *) item);
	wi_list_end(&c->list);
}


static void ui_rd_close(void *ctx)
{
	struct ui_rd_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


static void ui_rd_resume(void *ctx)
{
	ui_rd_close(ctx);
	ui_rd_open(ctx, NULL);
	progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_rd_events = {
	.touch_tap	= ui_rd_tap,
	.touch_long	= ui_rd_long,
	.touch_to	= ui_rd_to,
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_rd = {
	.name		= "rd",
	.ctx_size	= sizeof(struct ui_rd_ctx),
	.open		= ui_rd_open,
	.close		= ui_rd_close,
	.resume		= ui_rd_resume,
	.events		= &ui_rd_events,
};
