/*
 * wi_icons.c - Widget: array of tappable icons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "hal.h"
#include "gfx.h"
#include "shape.h"
#include "wi_icons.h"


#define	DEFAULT_BUTTON_R	12


static const struct wi_icons_style wi_icons_default_style = {
	.size		= 50,
	.button_r	= 12,
	.gap		= 12,
	.button_fg	= GFX_BLACK,
	.button_bg	= GFX_WHITE,
};


/* --- Helper functions ---------------------------------------------------- */


static unsigned coord(const struct wi_icons_style *style,
    unsigned c, unsigned i, unsigned n)
{
	return c - ((n - 1) / 2.0 - i) * (style->size + style->gap);
}


static unsigned xi(const struct wi_icons_style *style,
    unsigned cx, unsigned i, unsigned n)
{
	return coord(style, cx, i, n);
}


static unsigned yi(const struct wi_icons_style *style,
    unsigned cy, unsigned i, unsigned n)
{
	return coord(style, cy, i, n);
}


static void n_list_to_array(unsigned n, unsigned *nx, unsigned *ny)
{
	switch (n) {
	case 1:
	case 2:
	case 3:
		*nx = n;
		*ny = 1;
		break;
	case 4:
		*nx = 2;
		*ny = 2;
		break;
	case 5:
	case 6:
		*nx = 3;
		*ny = 2;
		break;
	case 7:
	case 8:
	case 9:
		*nx = 3;
		*ny = 3;
		break;
	default:
		ABORT();
	}
}


/* --- Draw icons ---------------------------------------------------------- */


void wi_icons_draw_access(struct gfx_drawable *da, unsigned cx, unsigned cy,
    const struct wi_icons_style *style,
    wi_icons_draw_fn access(void *user, unsigned i), void *user,
    unsigned n_icons)
{
	unsigned nx, ny, i, ix, iy;

	if (!style)
		style = &wi_icons_default_style;
	n_list_to_array(n_icons, &nx, &ny);
	i = 0;
	for (iy = 0; iy != ny; iy++)
		for (ix = 0; ix != nx; ix++) {
			if (i == n_icons)
				break;

			wi_icons_draw_fn fn = access(user, i);

			if (fn) {
				unsigned tx = xi(style, cx, ix, nx);
				unsigned ty = yi(style, cy, iy, ny);

				gfx_rrect_xy(da, tx - style->size / 2,
				    ty - style->size / 2,
				    style->size, style->size,
				    DEFAULT_BUTTON_R, style->button_bg);
				fn(da, style, tx, ty);
			}
			i++;
		}
}


static wi_icons_draw_fn ith_fn(void *user, unsigned i)
{
	const wi_icons_draw_fn *icons = user;

	return icons[i];
}


void wi_icons_draw(struct gfx_drawable *da, unsigned cx, unsigned cy,
    const struct wi_icons_style *style, const wi_icons_draw_fn *icons,
    unsigned n_icons)
{
	wi_icons_draw_access(da, cx, cy, style, ith_fn, (void *) icons,
	    n_icons);
}


/* --- Select icons by coordinates ----------------------------------------- */


int wi_icons_select(unsigned x, unsigned y, unsigned cx, unsigned cy,
    const struct wi_icons_style *style, unsigned n_icons)
{
	if (!style)
		style = &wi_icons_default_style;

	int d = (style->size + style->gap) / 2;
	unsigned nx, ny, i, ix, iy;

	n_list_to_array(n_icons, &nx, &ny);
	i = 0;
	for (iy = 0; iy != ny; iy++)
		for (ix = 0; ix != nx; ix++) {
			if (i == n_icons)
				break;

			int tx = xi(style, cx, ix, nx);
			int ty = yi(style, cy, iy, ny);

			if ((int) x >= tx - d && (int) x <= tx + d &&
			    (int) y >= ty - d && (int) y <= ty + d)
				return i;
			i++;
		}
	return -1;
}
