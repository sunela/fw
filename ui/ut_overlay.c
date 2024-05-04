/*
 * ut_overlay.c - User interface tool: Overlay buttons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdlib.h>

#include "hal.h"
#include "debug.h"
//#include "timer.h"
#include "gfx.h"
#include "shape.h"
#include "ui.h"
#include "ut_overlay.h"


#define	DEFAULT_BUTTON_R	12


static struct gfx_drawable old_da;
static gfx_color old_fb[GFX_WIDTH * GFX_HEIGHT];
static unsigned nx, ny;


static const struct ut_overlay_style default_style = {
	.size		= 50,
	.button_r	= 12,
	.gap		= 12,
	.halo		= 8,
	.button_fg	= GFX_BLACK,
	.button_bg	= GFX_WHITE,
	.halo_bg	= GFX_BLUE,
};


/* --- Wrappers for common symbols ----------------------------------------- */


void ui_overlay_sym_power(struct gfx_drawable *tmp_da,
    const struct ut_overlay_params *params, unsigned x, unsigned y, void *user)
{
	const struct ut_overlay_style *s =
	    params->style ? params->style : &default_style;

	gfx_power_sym(tmp_da, x, y, s->size * 0.3, s->size * 0.1,
	    s->button_fg, s->button_bg);
}


/* --- Event handling ------------------------------------------------------ */


static void ut_overlay_tap(unsigned x, unsigned y)
{
	ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void draw_button(struct gfx_drawable *tmp_da,
    const struct ut_overlay_params *p,
    const struct ut_overlay_button *b, int x, int y)
{
	const struct ut_overlay_style *s = p->style ? p->style : &default_style;

	gfx_rrect_xy(tmp_da, x - s->size / 2, y - s->size / 2,
	    s->size, s->size, DEFAULT_BUTTON_R, s->button_bg);
	if (b->draw)
		b->draw(tmp_da, p, x, y, b->user);
}


static void ut_overlay_open(void *params)
{
	const struct ut_overlay_params *p = params;
	const struct ut_overlay_button *b = p->buttons;
	const struct ut_overlay_style *s = p->style ? p->style : &default_style;
	struct gfx_drawable tmp_da;
	gfx_color tmp_fb[GFX_WIDTH * GFX_HEIGHT];
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
	w = nx * s->size + (nx - 1) * s->gap + 2 * s->halo;
	h = ny * s->size + (ny - 1) * s->gap + 2 * s->halo;
	gfx_rrect_xy(&tmp_da, (GFX_WIDTH - w) / 2, (GFX_HEIGHT - h) / 2, w, h,
	    r + s->halo, s->halo_bg);
	for (iy = 0; iy != ny; iy++)
		for (ix = 0; ix != nx; ix++) {
			unsigned x = GFX_WIDTH / 2 -
			    ((nx - 1) / 2.0 - ix) * (s->size + s->gap);
			unsigned y = GFX_HEIGHT / 2 -
			    ((ny - 1) / 2.0 - iy) * (s->size + s->gap);

			if (b == p->buttons + p->n_buttons)
				break;
			draw_button(&tmp_da, p, b, x, y);
			b++;
		}
	gfx_copy(&da, 0, 0, &tmp_da, 0, 0, da.w, da.h, GFX_TRANSPARENT);

//	timer_init(&t_button);
}


static void ut_overlay_close(void)
{
	gfx_copy(&da, 0, 0, &old_da, 0, 0, da.w, da.h, -1);
//	timer_cancel(&t_button);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ut_overlay_events = {
	.touch_tap	= ut_overlay_tap,
};


const struct ui ut_overlay = {
	.open = ut_overlay_open,
	.close = ut_overlay_close,
	.events	= &ut_overlay_events,
};
