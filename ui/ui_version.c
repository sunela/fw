/*
 * ui_version.c - User interface: Version information
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "hal.h"
#include "util.h"
#include "gfx.h"
#include "shape.h"
#include "text.h"
#include "wi_list.h"
#include "fmt.h"
#include "ui_overlay.h"
#include "style.h"
#include "colors.h"
#include "ui.h"
#include "version.h"


struct ui_version_ctx {
	struct wi_list list;
};

static const struct wi_list_style style = {
	.y0	= LIST_Y0,
	.y1	= GFX_HEIGHT - 1,
	.entry = {
		.fg	= { LIST_FG, LIST_FG },
		.bg	= { EVEN_BG, ODD_BG },
		.min_h	= 50,
	}
};

static struct wi_list *lists[1];


/* --- Long press ---------------------------------------------------------- */


static void power_off(void *user)
{
	turn_off();
}


static void version_overlay(struct ui_version_ctx *c)
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


static void ui_version_long(void *ctx, unsigned x, unsigned y)
{
	struct ui_version_ctx *c = ctx;

	version_overlay(c);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_version_open(void *ctx, void *params)
{
	struct ui_version_ctx *c = ctx;
	char tmp[20];
	char cpu_id[CPU_ID_LENGTH + 1];
	char *p;

	lists[0] = &c->list;

	gfx_rect_xy(&main_da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&main_da, GFX_WIDTH / 2, TOP_H / 2, "Version", &FONT_TOP,
	    GFX_CENTER, GFX_CENTER, GFX_WHITE);

	wi_list_begin(&c->list, &style);

	p = tmp;
	format(add_char, &p, "%08lx%s",
	    (unsigned long) (build_override ? STATIC_BUILD_HASH : build_hash),
	    (build_override ? STATIC_BUILD_DIRTY : build_dirty) ? "+" : "");
	wi_list_add(&c->list, "Firmware", tmp, NULL);

	wi_list_add(&c->list, "Build date",
	    build_override ? STATIC_BUILD_DATE : build_date, NULL);

	read_cpu_id(cpu_id);
	wi_list_add(&c->list, "CPU", cpu_id, NULL);

	wi_list_end(&c->list);
}


static void ui_version_close(void *ctx)
{
	struct ui_version_ctx *c = ctx;

	wi_list_destroy(&c->list);
}


static void ui_version_resume(void *ctx)
{
	ui_version_close(ctx);
	ui_version_open(ctx, NULL);
	progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_version_events = {
	.touch_long	= ui_version_long,
	.touch_to	= swipe_back,
	.lists		= lists,
	.n_lists	= 1,
};

const struct ui ui_version = {
	.name		= "version",
	.ctx_size	= sizeof(struct ui_version_ctx),
	.open		= ui_version_open,
	.close		= ui_version_close,
	.resume		= ui_version_resume,
	.events		= &ui_version_events,
};
