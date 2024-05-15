/*
 * ui_overlay.c - User interface: Overlay buttons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdlib.h>

#include "hal.h"
#include "debug.h"
#include "timer.h"
#include "gfx.h"
#include "shape.h"
#include "ui.h"
#include "ui_overlay.h"


#define	MAX_BUTTONS		9

#define	DEFAULT_BUTTON_R	12


struct button_ref {
	void (*fn)(void *user);	/* skip if NULL */
	void *user;
	struct gfx_rect bb;
};


static struct gfx_drawable old_da;
static PSRAM gfx_color old_fb[GFX_WIDTH * GFX_HEIGHT];
static PSRAM gfx_color tmp_fb[GFX_WIDTH * GFX_HEIGHT];
static struct button_ref refs[MAX_BUTTONS];


static const struct ui_overlay_style default_style = {
	.size		= 50,
	.button_r	= 12,
	.gap		= 12,
	.halo		= 8,
	.button_fg	= GFX_BLACK,
	.button_bg	= GFX_WHITE,
	.halo_bg	= GFX_BLACK,
};

static struct timer t_overlay_idle;


/* --- Wrappers for common symbols ----------------------------------------- */


void ui_overlay_sym_power(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_power_sym(tmp_da, x, y, style->size * 0.3, style->size * 0.1,
	    style->button_fg, style->button_bg);
}


void ui_overlay_sym_delete(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_diagonal_cross(tmp_da, x, y, style->size * 0.4, style->size * 0.1,
	    style->button_fg);
}


void ui_overlay_sym_add(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;
	unsigned side = style->size * 0.7;

	gfx_greek_cross(tmp_da, x - side / 2, y - side / 2,
	    side, side, style->size * 0.15, style->button_fg);
}


void ui_overlay_sym_back(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_equilateral(tmp_da, x + style->size * 0.05, y, style->size * 0.7,
	    -1, style->button_fg);
}


void ui_overlay_sym_next(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_equilateral(tmp_da, x - style->size * 0.05, y, style->size * 0.7,
	    1, style->button_fg);
}


void ui_overlay_sym_edit(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;
	unsigned side;

	side = gfx_pencil_sym(NULL, 0, 0,
	    style->size * 0.4, style->size * 0.7,	// wi len
	     style->size * 0.1,				// lw
	    0, 0);
	gfx_pencil_sym(tmp_da, x - side * 0.45, y - side / 2,
	    style->size * 0.4, style->size * 0.7,	// wi len
	     style->size * 0.1,				// lw
	    style->button_fg, style->button_bg);
}


void ui_overlay_sym_setup(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_gear_sym(tmp_da, x, y,
	    style->size * 0.3, style->size * 0.1,	// ro ri
	    style->size * 0.2, style->size * 0.15,	// tb tt
	    style->size * 0.1,				// th
	    style->button_fg, style->button_bg);
}


static void sym_move(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y,
    bool from, int to)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_move_sym(tmp_da, x, y,
	    style->size * 0.4, style->size * 0.15,	// bs br
	    style->size * 0.1,				// lw
	    from, to, style->button_fg, style->button_bg);
}


void ui_overlay_sym_move_from(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	sym_move(tmp_da, params, x, y, 1, 0);
}


void ui_overlay_sym_move_to(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	sym_move(tmp_da, params, x, y, 0, 1);
}


void ui_overlay_sym_move_cancel(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user)
{
	sym_move(tmp_da, params, x, y, 1, -1);
}


/* --- Event handling ------------------------------------------------------ */


static void ui_overlay_tap(unsigned x, unsigned y)
{
	const struct button_ref *ref;

	for (ref = refs; ref != refs + MAX_BUTTONS; ref++) {
		if (!ref->fn)
			continue;
		if (x < ref->bb.x || x >= ref->bb.x + ref->bb.w)
			continue;
		if (y < ref->bb.y || y >= ref->bb.y + ref->bb.h)
			continue;
		ref->fn(ref->user);
		return;
	}
	ui_return();
}


static void ui_overlay_cancel(void)
{
	ui_return();
}


/* --- Idle timer ---------------------------------------------------------- */


static void overlay_idle(void *user)
{
	ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void draw_button(struct gfx_drawable *tmp_da,
    const struct ui_overlay_params *p,
    const struct ui_overlay_button *b, int x, int y)
{
	const struct ui_overlay_style *style =
	    p->style ? p->style : &default_style;

	gfx_rrect_xy(tmp_da, x - style->size / 2, y - style->size / 2,
	    style->size, style->size, DEFAULT_BUTTON_R, style->button_bg);
	if (b->draw)
		b->draw(tmp_da, p, x, y, b->user);
}


static void ui_overlay_open(void *params)
{
	const struct ui_overlay_params *p = params;
	const struct ui_overlay_button *b = p->buttons;
	const struct ui_overlay_style *style =
	    p->style ? p->style : &default_style;
	struct button_ref *ref = refs;
	struct gfx_drawable tmp_da;
	unsigned nx, ny;
	unsigned w, h, ix, iy;
	unsigned r = DEFAULT_BUTTON_R;

	gfx_da_init(&old_da, da.w, da.h, old_fb);
	gfx_da_init(&tmp_da, da.w, da.h, tmp_fb);
	gfx_copy(&old_da, 0, 0, &da, 0, 0, da.w, da.h, -1);
	gfx_clear(&tmp_da, GFX_TRANSPARENT);

	switch (p->n_buttons) {
	case 1:
		nx = 1;
		ny = 1;
		break;
	case 2:
		nx = 2;
		ny = 1;
		break;
	case 3:
		nx = 3;
		ny = 1;
		break;
	case 4:
		nx = 2;
		ny = 2;
		break;
	case 5:
	case 6:
		nx = 3;
		ny = 2;
		break;
	case 7:
	case 8:
	case 9:
		nx = 3;
		ny = 3;
		break;
	default:
		abort();
	}
	w = nx * style->size + (nx - 1) * style->gap + 2 * style->halo;
	h = ny * style->size + (ny - 1) * style->gap + 2 * style->halo;
	gfx_rrect_xy(&tmp_da, (GFX_WIDTH - w) / 2, (GFX_HEIGHT - h) / 2, w, h,
	    r + style->halo, style->halo_bg);
	for (iy = 0; iy != ny; iy++)
		for (ix = 0; ix != nx; ix++) {
			unsigned x = GFX_WIDTH / 2 -
			    ((nx - 1) / 2.0 - ix) * (style->size + style->gap);
			unsigned y = GFX_HEIGHT / 2 -
			    ((ny - 1) / 2.0 - iy) * (style->size + style->gap);

			if (b == p->buttons + p->n_buttons)
				break;
			if (b->draw) {
				draw_button(&tmp_da, p, b, x, y);
				ref->fn = b->fn;
				ref->user = b->user;
				ref->bb.x =
				    x - style->size / 2 - style->gap / 2;
				ref->bb.y =
				    y - style->size / 2 - style->gap / 2;
				ref->bb.w = ref->bb.h =
				    style->size + style->gap;
				ref++;
			}
			b++;
		}
	gfx_copy(&da, 0, 0, &tmp_da, 0, 0, da.w, da.h, GFX_TRANSPARENT);

	while (ref != refs + MAX_BUTTONS) {
		ref->fn = NULL;
		ref++;
	}

	timer_init(&t_overlay_idle);
	timer_set(&t_overlay_idle, 1000 * IDLE_OVERLAY_S, overlay_idle, NULL);

	/* make sure we don't hit the caller's idle timer */
	progress();
}


static void ui_overlay_close(void)
{
	gfx_copy(&da, 0, 0, &old_da, 0, 0, da.w, da.h, -1);
	timer_cancel(&t_overlay_idle);
	progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_overlay_events = {
	.touch_tap	= ui_overlay_tap,
	.touch_cancel	= ui_overlay_cancel,
};


const struct ui ui_overlay = {
	.open = ui_overlay_open,
	.close = ui_overlay_close,
	.events	= &ui_overlay_events,
};
