/*
 * wi_icons.c - Widget: array of tappable icons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "gfx.h"
#include "shape.h"
#include "ui_overlay.h"
#include "wi_icons.h"


#define	DEFAULT_BUTTON_R	12


static unsigned coord(const struct ui_overlay_style *style,
    unsigned c, unsigned i, unsigned n)
{
	return c - ((n - 1) / 2.0 - i) * (style->size + style->gap);
}


static unsigned ix(const struct ui_overlay_style *style,
    unsigned cx, unsigned i, unsigned n)
{
	return coord(style, cx, i, n);
}


static unsigned iy(const struct ui_overlay_style *style,
    unsigned cy, unsigned i, unsigned n)
{
	return coord(style, cy, i, n);
}


void wi_icons_draw(struct gfx_drawable *da, unsigned cx, unsigned cy,
    const struct ui_overlay_style *style, const wi_icons_draw_fn *icons,
    unsigned n_icons)
{
	unsigned i;

	if (!style)
		style = &ui_overlay_default_style;
	for (i = 0; i != n_icons; i++)
		if (icons[i]) {
			unsigned tx = ix(style, cx, i, n_icons);
			unsigned ty = iy(style, cy, 0, 1);

			gfx_rrect_xy(da, tx - style->size / 2,
			    ty - style->size / 2, style->size, style->size,
			    DEFAULT_BUTTON_R, style->button_bg);
			icons[i](da, style, tx, ty);
		}
}


int wi_icons_select(unsigned x, unsigned y, unsigned cx, unsigned cy,
    const struct ui_overlay_style *style, unsigned n_icons)
{
	if (!style)
		style = &ui_overlay_default_style;

	unsigned d = (style->size + style->gap) / 2;
	unsigned i;

	for (i = 0; i != n_icons; i++) {
		unsigned tx = ix(style, cx, i, n_icons);
		unsigned ty = iy(style, cy, 0, 1);

		if (x >= tx - d && x <= tx + d && y >= ty - d && y <= ty + d)
			return i;
	}
	return -1;
}
