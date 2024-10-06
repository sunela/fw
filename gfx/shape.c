/*
 * shape.c - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <string.h>
#include <assert.h>

#include "imath.h"
#include "gfx.h"
#include "shape.h"


//#define DEBUG

#define	SQRT_2	1.414
#define	PI	3.1416


/* --- Diagonal cross ------------------------------------------------------ */


void gfx_diagonal_cross(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned r, unsigned lw, gfx_color color)
{
	unsigned d0 = (r - lw) / SQRT_2;
	unsigned d1 = (r + lw) / SQRT_2;
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


/* --- Greek cross (plus sign) --------------------------------------------- */


void gfx_greek_cross(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned w, unsigned h, unsigned lw, gfx_color color)
{
	gfx_rect_xy(da, x, y + h / 2- lw / 2, w, lw, color);
	gfx_rect_xy(da, x + w / 2 - lw / 2, y, lw, h, color);
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


static void point_on_arc(short **p, int x, int y, unsigned r,
    unsigned a)
{
	uint8_t pos = isin(a % 45, r);
	int ox, oy, dx, dy;

	octant(&ox, &oy, &dx, &dy, a / 45);
	*(*p)++ = x + ox * r + dx * pos;
	*(*p)++ = y - oy * r - dy * pos;
}


void gfx_arc(struct gfx_drawable *da, int x, int y, unsigned r,
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


unsigned gfx_power_sym(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r,
    unsigned lw, gfx_color color, gfx_color bg)
{
	unsigned or;	/* outer radius */
	unsigned ey = isqrt(r * r - 4 * lw * lw);
	float f = 2.0 * lw / ey;
	int i;

	lw |= 1;	/* we need this for rounded caps */
	or = r + lw / 2;

	if (da) {
		/* broken circle */

		gfx_disc(da, x, y, or, color);
		gfx_disc(da, x, y, r - lw / 2, bg);
		gfx_triangle(da, x, y, x - f * or, y - or, x + f * or, y - or,
		    bg);
		for (i = -1; i <= 1; i += 2)
			gfx_disc(da, x + i * lw * 2, y - ey, lw / 2, color);

		/* verical bar */

		gfx_rect_xy(da, x - lw / 2, y - BAR_TOP(r, lw),
		    lw, BAR_H(r, lw), color);
		gfx_disc(da, x, y - BAR_TOP(r, lw), lw / 2, color);
		gfx_disc(da, x, y - BAR_BOTTOM(r, lw), lw / 2, color);
	}

	return BAR_TOP(r, lw) + lw / 2;
}


/* --- Pencil (edit symbol) ------------------------------------------------ */


unsigned gfx_pencil_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned width, unsigned length, unsigned lw,
    gfx_color color, gfx_color bg)
{
	unsigned d = width / SQRT_2;
	unsigned side = length / SQRT_2 + d;
	short vo[10] = {
		x + side - 1,		y + d ,
		x + side - 1 - d,	y,
		x,			y + side - 1 - d,
		x,			y + side - 1,
		x + d,			y + side - 1
	};
	short vi[8] = {
		vo[0] - lw * SQRT_2,	vo[1],
		vo[2],			vo[3] + lw * SQRT_2,
		vo[4] + lw / SQRT_2,	vo[5] + lw / SQRT_2,
		vo[8] - lw / SQRT_2,	vo[9] - lw / SQRT_2
	};

	if (da) {
		gfx_poly(da, 5, vo, color);
		gfx_poly(da, 4, vi, bg);
	}

	return side;
}


/* --- Gear (setup (symbol) ------------------------------------------------ */


/*
 * https://en.wikipedia.org/wiki/Rotation_matrix
 * https://en.wikipedia.org/wiki/Exact_trigonometric_values
 * https://en.wikipedia.org/wiki/Matrix_multiplication
 */

static const float matrix_identity[2][2] = { { 1, 0 }, { 0, 1 }};
static const float matrix_45deg[2][2] =
    { { SQRT_2 / 2, -SQRT_2 / 2 }, { SQRT_2 / 2, SQRT_2 / 2 }};


static void matrix_mult(float res[2][2], const float a[2][2],
    const float b[2][2])
{
	unsigned i, j, k;

	for (i = 0; i != 2; i++)
		for (j = 0; j != 2; j++) {
			res[i][j] = 0;
			for (k = 0; k != 2; k++)
				res[i][j] += a[i][k] * b[k][j];
		}
}


void gfx_gear_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned ro, unsigned ri, unsigned tb, unsigned tt, unsigned th,
    gfx_color color, gfx_color bg)
{
	float m[2][2];
	int rb = isqrt(ro * ro - tb * tb / 4);
	short trap[] = {
		-tb / 2,	rb,
		-tt / 2,	ro + th,
		tt / 2,		ro + th,
		tb / 2,		rb,
	};
	unsigned o, i;

	memcpy(m, matrix_identity, sizeof(m));
	gfx_disc(da, x, y, ro, color);
	x++; /* @@@ for gfx_poly weirdness */
	for (o = 0; o != 8; o++) {
		float tmp[2][2];
		short v[8];

		for (i = 0; i != 8; i += 2) {
			v[i] = x + trap[i] * m[0][0] + trap[i + 1] * m[0][1];
			v[i + 1] =
			    y + trap[i] * m[1][0] + trap[i + 1] * m[1][1];
		}
		gfx_poly(da, 4, v, color);
		matrix_mult(tmp, m, matrix_45deg);
		memcpy(m, tmp, sizeof(m));
	}
	x--; /* @@@ */
	gfx_disc(da, x, y, ri, bg);
}


/* --- Move symbols -------------------------------------------------------- */


#define	TWEAK	1	/* @@@ ugly little micro-adjustments */


void gfx_move_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned box_size, unsigned box_ro, unsigned lw,
    bool from, int to, gfx_color color, gfx_color bg)
{
	unsigned r = (box_size + lw) / 2;

	/* arc */
	gfx_arc(da, x, y - lw, r + lw / 2, 270, 90, color, bg);
	gfx_disc(da, x, y - lw, r - lw / 2, bg);

	/* left box */
	gfx_rrect_xy(da, x - box_size - lw / 2, y, box_size, box_size,
	    box_ro, color);
	if (!from)
		gfx_rrect_xy(da, x - box_size + lw / 2, y + lw,
		    box_size - 2 * lw, box_size - 2 * lw, box_ro - lw, bg);

	/* right box, cross, and arrowhead */
	if (to < 0) {
		gfx_diagonal_cross(da, x + r + TWEAK, y,
		    box_size / 2, lw / 2, color);
	} else {
		gfx_rrect_xy(da, x + lw / 1.414, y, box_size, box_size,
		    box_ro, color);
		if (!to)
			gfx_rrect_xy(da, x + 3 * lw / 2, y + lw,
			    box_size - 2 * lw, box_size - 2 * lw,
			    box_ro - lw, bg);

		/* point of arrowhead */
		unsigned ax = x + r;
		unsigned ay = y - lw / 4;

		gfx_triangle(da, ax + TWEAK, ay,
		    ax - lw, ay - 1.5 * lw, ax + lw + TWEAK, ay - 1.5 * lw,
		    color);
	}
}


/* --- Checkbox ------------------------------------------------------------ */


void gfx_checkbox(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned w, unsigned lw, bool on, gfx_color color, gfx_color bg)
{
	unsigned iw = w - 2 * lw;
	unsigned cw = lw * 1.414 + 1;	/* sqrt(2), crudely rounded up */
	unsigned i;

	assert(lw < w / 2);
	gfx_rect_xy(da, x - w / 2, y - w / 2, w, w, color);
	gfx_rect_xy(da, x - iw / 2, y - iw / 2, iw, iw, bg);
	if (on)
		for (i = 0; i != iw; i++) {
			gfx_rect_xy(da, x - iw / 2 - cw / 2 + i,
			    y - iw / 2 + i, cw, 1, color);
			gfx_rect_xy(da, x + (iw + 1) / 2 - (cw + 1) / 2 - i,
			    y - iw / 2 + i, cw, 1, color);
		}
}


/* --- PC communication ---------------------------------------------------- */


static void rrect_centered(struct gfx_drawable *da, unsigned cx, unsigned cy,
    unsigned w, unsigned h, unsigned r, gfx_color color)
{
	gfx_rrect_xy(da, cx - w  / 2, cy - h / 2, w, h, r, color);
}


/*
 * @@@ The coefficients here have been chosen to produce a good-looking icon
 * with the default size, but may produce much less pleasing results at
 * different sizes. For rectangles inside others, we should calculate the
 * border width, and size the rectangle such that all borders uses that width,
 * instead of hoping that multiplying the size (with rounding) will yield the
 * symmetry.
 */

void gfx_pc_comm_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned side, gfx_color color, gfx_color bg)
{
	unsigned ro = side * 0.07;
	unsigned cx = x - side * 0.30;
	unsigned cy = y + side * 0.22;

	/* monitor */
	rrect_centered(da, x, y, side * 0.8, side * 0.7, ro, color);
	gfx_rect_xy(da,
	    x - side * 0.3, y - side * 0.25, side * 0.6, side * 0.45, bg);
#if 0
	gfx_rect_xy(da,
	    x - side * 0.2, y + side * 0.35, side * 0.4, side * 0.1, color);
#endif

	/* sunela */

	ro = side * 0.08;

	rrect_centered(da, cx, cy, side * 0.3 + 2, side * 0.4 + 2, ro, bg);
	rrect_centered(da, cx, cy, side * 0.3, side * 0.4, ro, color);
	rrect_centered(da, cx, cy, side * 0.2, side * 0.3, side * 0.07, bg);
}


/* --- Folder -------------------------------------------------------------- */


void gfx_folder(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned w, unsigned h, unsigned rider_w, unsigned rider_h, unsigned r,
    gfx_color color)
{
	unsigned i;

	gfx_rrect_xy(da, x, y, rider_w, h, r, color);
	gfx_rrect_xy(da, x, y + rider_h, w, h - rider_h, r, color);
	for (i = 0; i != rider_h; i++)
		gfx_rect_xy(da, x + rider_w - r, y + i, i + r, 1, color);
}


void gfx_folder_outline(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned w, unsigned h, unsigned rider_w, unsigned rider_h, unsigned r,
    unsigned lw, gfx_color color, gfx_color bg)
{
	unsigned ri = lw > r ? 0 : r - lw;

	gfx_folder(da, x, y, w, h, rider_w, rider_h, r, color);
	gfx_folder(da, x + lw, y + lw, w - 2 * lw, h - 2 * lw,
	    rider_w - 1.414 * lw, rider_h, ri, bg);
}
