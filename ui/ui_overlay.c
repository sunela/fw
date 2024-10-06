/*
 * ui_overlay.c - User interface: Overlay buttons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ ui_overlay is designed to preserve the screen content of the page it was
 * called from and to use it as background. However, ui_call always clears the
 * screen before switching, so the icons of the overlay appear on an all-black
 * background.
 *
 * Surprisingly, it took me weeks to even realize this was happening. This
 * raises the question whether it even makes sense to try to preserve the
 * previous content as background, or whether the way it is now actually looks
 * good enough, or even better.
 *
 * Regarding the previous screen content, we have three options:
 *
 * 1) just clear the screen, as we currently do.
 *
 * 2) preserve the screen, as ui_overlay is designed to support. (Just removing
 *    the gfx_clear in ui_call enabled this, but there are a few places where
 *    ui_call is used where we don't want the old screen content to stay
 *    around, so a bit more work would be needed to do this properly.)
 *
 *    One issue is that buttons for which space is allocated but that aren't
 *    shown still have their background blacked out, which looks a bit odd.
 *    Possible solutions:
 *    a) make the "halo" follow the outline of the buttons shown. This is
 *       somewhat difficult.
 *    b) instead of one large "halo" around the overlay, draw one around each
 *       (visible) button. This is easy to do, and may look almost like a).
 *    c) draw an outline around the buttons, visually delimiting the overlay
 *       area.
 *    d) use a non-black background for the overlay, also visually delimiting
 *       the overlay area.
 *
 * 3) preserve the screen, but reduce the brightness of the old content. E.g.,
 *    divide all pixel values by two. This may look good, too.
 */

#include <stddef.h>

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


static struct button_ref refs[MAX_BUTTONS];


static const struct ui_overlay_style default_style = {
	.size		= 50,
	.button_r	= 12,
	.gap		= 12,
	.button_fg	= GFX_BLACK,
	.button_bg	= GFX_WHITE,
};

static struct timer t_overlay_idle;


/* --- Draw a button with symbol ------------------------------------------- */


static void draw_button(struct gfx_drawable *da,
    const struct ui_overlay_params *p,
    const struct ui_overlay_button *b, int x, int y)
{
	const struct ui_overlay_style *style =
	    p->style ? p->style : &default_style;

	gfx_rrect_xy(da, x - style->size / 2, y - style->size / 2,
	    style->size, style->size, DEFAULT_BUTTON_R, style->button_bg);
	if (b->draw)
		b->draw(da, p, x, y);
}


/* --- Lend some buttons to other pages ------------------------------------ */


bool button_in(unsigned cx, unsigned cy, unsigned x, unsigned y)
{
	unsigned r = (default_style.size + default_style.gap) / 2;

	return x >= cx - r && x <= cx + r && y >= cy - r && y <= cy + r;
}


void button_draw_add(unsigned x, unsigned y)
{
	static const struct ui_overlay_params params;
	static const struct ui_overlay_button button =
	    { .draw = ui_overlay_sym_add };

	draw_button(&main_da, &params, &button, x, y);
}


/* --- Wrappers for common symbols ----------------------------------------- */


void ui_overlay_sym_power(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_power_sym(da, x, y, style->size * 0.3, style->size * 0.1,
	    style->button_fg, style->button_bg);
}


void ui_overlay_sym_delete(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_diagonal_cross(da, x, y, style->size * 0.4, style->size * 0.1,
	    style->button_fg);
}


void ui_overlay_sym_add(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;
	unsigned side = style->size * 0.7;

	gfx_greek_cross(da, x - side / 2, y - side / 2,
	    side, side, style->size * 0.15, style->button_fg);
}


void ui_overlay_sym_back(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_equilateral(da, x + style->size * 0.05, y, style->size * 0.7,
	    -1, style->button_fg);
}


void ui_overlay_sym_next(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_equilateral(da, x - style->size * 0.05, y, style->size * 0.7,
	    1, style->button_fg);
}


void ui_overlay_sym_edit(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;
	unsigned side;

	side = gfx_pencil_sym(NULL, 0, 0,
	    style->size * 0.4, style->size * 0.7,	// wi len
	    style->size * 0.1,				// lw
	    0, 0);
	gfx_pencil_sym(da, x - side * 0.45, y - side / 2,
	    style->size * 0.4, style->size * 0.7,	// wi len
	    style->size * 0.1,				// lw
	    style->button_fg, style->button_bg);
}


void ui_overlay_sym_setup(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_gear_sym(da, x, y,
	    style->size * 0.3, style->size * 0.1,	// ro ri
	    style->size * 0.2, style->size * 0.15,	// tb tt
	    style->size * 0.1,				// th
	    style->button_fg, style->button_bg);
}


static void sym_move(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y,
    bool from, int to)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_move_sym(da, x, y,
	    style->size * 0.4, style->size * 0.15,	// bs br
	    style->size * 0.1,				// lw
	    from, to, style->button_fg, style->button_bg);
}


void ui_overlay_sym_move_from(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	sym_move(da, params, x, y, 1, 0);
}


void ui_overlay_sym_move_to(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	sym_move(da, params, x, y, 0, 1);
}


void ui_overlay_sym_move_cancel(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	sym_move(da, params, x, y, 1, -1);
}


void ui_overlay_sym_pc_comm(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;

	gfx_pc_comm_sym(da, x, y, style->size * 0.8,
	    style->button_fg, style->button_bg);
}


void ui_overlay_sym_folder(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y)
{
	const struct ui_overlay_style *style =
	    params->style ? params->style : &default_style;
	unsigned w = style->size * 0.7;
	unsigned h = style->size * 0.5;
	unsigned lw = 4;
#if 0
	unsigned ro = h / 9;
	int ri = ro - lw;

	gfx_folder(da, x - w / 2, y - h / 2, w, h, w / 2, h / 5, ro,
	    style->button_fg);
	gfx_folder(da, x - w / 2 + lw, y - h / 2 + lw, w - 2 * lw, h - 2 * lw,
	    w / 2 - 1.414 * lw, h / 5, ri < 0 ? 0 : ri,
	    style->button_bg);
#endif
	gfx_folder_outline(da, x - w / 2, y - h / 2, w, h, w / 2, h / 5, h / 9,
	    lw, style->button_fg, style->button_bg);
}


/* --- Event handling ------------------------------------------------------ */


static void ui_overlay_tap(void *ctx, unsigned x, unsigned y)
{
	const struct button_ref *ref;

	for (ref = refs; ref != refs + MAX_BUTTONS; ref++) {
		if (!ref->fn)
			continue;
		if ((int) x < ref->bb.x || (int) x >= ref->bb.x + ref->bb.w)
			continue;
		if ((int) y < ref->bb.y || (int) y >= ref->bb.y + ref->bb.h)
			continue;
		ref->fn(ref->user);
		return;
	}
	ui_return();
}


static void ui_overlay_cancel(void *ctx)
{
	ui_return();
}


/* --- Idle timer ---------------------------------------------------------- */


static void overlay_idle(void *user)
{
	ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_overlay_open(void *ctx, void *params)
{
	const struct ui_overlay_params *p = params;
	const struct ui_overlay_button *b = p->buttons;
	const struct ui_overlay_style *style =
	    p->style ? p->style : &default_style;
	struct button_ref *ref = refs;
	unsigned nx, ny;
	unsigned ix, iy;

	gfx_clear(&main_da, GFX_BLACK);

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
		ABORT();
	}
	for (iy = 0; iy != ny; iy++)
		for (ix = 0; ix != nx; ix++) {
			unsigned x = GFX_WIDTH / 2 -
			    ((nx - 1) / 2.0 - ix) * (style->size + style->gap);
			unsigned y = GFX_HEIGHT / 2 -
			    ((ny - 1) / 2.0 - iy) * (style->size + style->gap);

			if (b == p->buttons + p->n_buttons)
				break;
			if (b->draw) {
				draw_button(&main_da, p, b, x, y);
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

	while (ref != refs + MAX_BUTTONS) {
		ref->fn = NULL;
		ref++;
	}

	timer_init(&t_overlay_idle);
	timer_set(&t_overlay_idle, 1000 * IDLE_OVERLAY_S, overlay_idle, NULL);

	/* make sure we don't hit the caller's idle timer */
	progress();
}


static void ui_overlay_close(void *ctx)
{
	timer_cancel(&t_overlay_idle);
	progress();
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_overlay_events = {
	.touch_tap	= ui_overlay_tap,
	.touch_cancel	= ui_overlay_cancel,
};


const struct ui ui_overlay = {
	.name		= "overlay",
	.open		= ui_overlay_open,
	.close		= ui_overlay_close,
	.events		= &ui_overlay_events,
};
