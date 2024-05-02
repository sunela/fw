/*
 * shape.c - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "gfx.h"
#include "shape.h"


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
