/*
 * shape.c - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <assert.h>

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


/* --- Filled arc ---------------------------------------------------------- */


static const uint8_t sin_tab[] = {
#include "sin.inc"
};


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
	uint8_t pos = r * sin_tab[a % 45] / 255;
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
