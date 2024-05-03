/*
 * shape.c - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <assert.h>

#include "imath.h"
#include "gfx.h"
#include "shape.h"


//#define DEBUG


/* --- Diagonal cross ------------------------------------------------------ */


void gfx_cross(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r,
    unsigned w, gfx_color color)
{
	unsigned d0 = (r - w) / 1.414;
	unsigned d1 = (r + w) / 1.414;
	short v1[] = {
		x + d0, y - d1,
		x + d1, y - d0,
		x - d0, y + d1,
		x - d1, y + d0
	};
	short v2[] = {
		x - d0, y - d1,
		x - d1, y - d0,
		x + d0, y + d1,
		x + d1, y + d0
	};

	gfx_poly(da, 4, v1, color);
	gfx_poly(da, 4, v2, color);
}


/* --- Filled triangle ----------------------------------------------------- */


void gfx_triangle(struct gfx_drawable *da, unsigned x0, unsigned y0,
    unsigned x1, unsigned y1, unsigned x2, unsigned y2, gfx_color color)
{
	short v[] = { x0, y0, x1, y1, x2, y2 };

	gfx_poly(da, 3, v, color);
}


/* --- Equilateral triangle ------------------------------------------------ */


void gfx_equilateral(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned a, int dir, gfx_color color)
{
	/* R = a / sqrt(3); R = 2 r */
	unsigned R = a / 1.732;
	unsigned r = R / 2;
	short v[] = {
		x - dir * r, y - a / 2,
		x - dir * r, y + a / 2,
		x + dir * R, y
	};

	gfx_poly(da, 3, v, color);
}


/* --- Filled rectangle with rounded corners ------------------------------- */


void gfx_rrect_xy(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h, unsigned r, gfx_color color)
{
	int dx, dy;

	if (2 * r > h)
		r = h / 2;
	if (2 * r > w)
		r = w / 2;
	for (dx = 0; dx != 2; dx++)
                for (dy = 0; dy != 2; dy++)
                        gfx_disc(da,
			    x + dx * (w - 1) * dx + r * (1 - dx * 2),
			    y + dy * (h - 1) * dy + r * (1 - dy * 2), r, color);
	gfx_rect_xy(da, x + r, y, w - 2 * r, r, color);
	gfx_rect_xy(da, x, y + r, w, h - 2 * r, color);
	gfx_rect_xy(da, x + r, y + h - r, w - 2 * r, r, color);
}


void gfx_rrect(struct gfx_drawable *da, const struct gfx_rect *bb, unsigned r,
    gfx_color color)
{
	gfx_rrect_xy(da, bb->x, bb->y, bb->w, bb->h, r, color);
}


/* --- Filled arc ---------------------------------------------------------- */


static void octant(int *ox, int *oy, int *dx, int *dy, uint8_t oct)
{
	static int8_t d[] = { 0, 1, 1, 1, 0, -1, -1, -1 };

	*ox = d[oct];
	*oy = d[(oct + 2) & 7];
	*dx = d[(oct + 1) & 7] - *ox;
	*dy = d[(oct + 3) & 7] - *oy;
}


static void point_on_arc(short **p, unsigned x, unsigned y, unsigned r,
    unsigned a)
{
	uint8_t pos = r * isin(a % 45, r);
	int ox, oy, dx, dy;

	octant(&ox, &oy, &dx, &dy, a / 45);
	*(*p)++ = x + ox * r + dx * pos;
	*(*p)++ = y - oy * r - dy * pos;
}


void gfx_arc(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r,
    unsigned a0, unsigned a1, gfx_color color, gfx_color bg)
{
	short v[22];	/* center, arc start, end, and up to 8 octants */
	short *p = v;
	unsigned oct;

#if DEBUG
	gfx_rect_xy(da, x - r, y - r, 2 * r + 1, 2 * r + 1, GFX_RED);
#endif
	gfx_disc(da, x, y, r, color);
	if (a0 == a1)
		return;

	/* @@@ for some weird reason, gfx_triangle (gfx_poly) is slightly off */
	x++;
	r++;

	if (a0 / 45 != a1 / 45 || a0 < a1)
		for (oct = a1 / 45; p == v || oct != a0 / 45;
		    oct = (oct + 1) & 7) {
			int ox, oy, dx, dy;

			/* add end of octant */
			octant(&ox, &oy, &dx, &dy, oct);
			*p++ = x + ox * r + dx * r;
			*p++ = y - oy * r - dy * r;
		}

	point_on_arc(&p, x, y, r, a0);
	*p++ = x;
	*p++ = y;
	point_on_arc(&p, x, y, r, a1);

	assert(p - v >= 6);
	assert(p - v <= 22);

	gfx_poly(da, (p - v) / 2, v, bg);
}


/* --- Power symbol -------------------------------------------------------- */


/*
 * Origin of the power symbol:
 * https://en.wikipedia.org/wiki/Power_symbol
 *
 * We draw the arc such that the gap between the ends of the arc (with rounded
 * cap) and the vertical bar is equal to the line width. The endpoints of the
 * arc are therefore, if the arc is centered at (0, 0), at x = +/- 2 * lw,
 * y = sqrt(r ^ 2 - 4 * lw ^ 2)
 */

#define	BAR_TOP(r, lw)	((r) + (lw) / 2)
#define	BAR_BOTTOM(r, lw) ((r) / 4)
#define	BAR_H(r, lw)	(BAR_TOP(r, lw) - BAR_BOTTOM(r, lw) - 1)


void gfx_power_sym(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r,
    unsigned lw, gfx_color color, gfx_color bg)
{
	unsigned or;	/* outer radius */
	unsigned ey = isqrt(r * r - 4 * lw * lw);
	float f = 2.0 * lw / ey;
	int i;

	lw |= 1;	/* we need this for rounded caps */
	or = r + lw / 2;

	/* broken circle */

	gfx_disc(da, x, y, or, color);
	gfx_disc(da, x, y, r - lw / 2, bg);
	gfx_triangle(da, x, y, x - f * or, y - or, x + f * or, y - or, bg);
	for (i = -1; i <= 1; i += 2)
		gfx_disc(da, x + i * lw * 2, y - ey, lw / 2, GFX_WHITE);

	/* verical bar */

	gfx_rect_xy(da, x - lw / 2, y - BAR_TOP(r, lw), lw, BAR_H(r, lw),
	    color);
	gfx_disc(da, x, y - BAR_TOP(r, lw), lw / 2, color);
	gfx_disc(da, x, y - BAR_BOTTOM(r, lw), lw / 2, color);
}
