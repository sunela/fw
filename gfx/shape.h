/*
 * shape.h - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SHAPE_H
#define	SHAPE_H

#include "gfx.h"


/*
 * Diagonal cross centered at (x, y), with radius r.
 */

void gfx_diagonal_cross(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned r, unsigned lw, gfx_color color);

/*
 * Cross with arm lengths w and h (for w = h, this is a Greek cross), with the
 * upper left corner at (x, y).
 */

void gfx_greek_cross(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned w, unsigned h, unsigned lw, gfx_color color);;

void gfx_triangle(struct gfx_drawable *da, unsigned x0, unsigned y0,
    unsigned x1, unsigned y1, unsigned x2, unsigned y2, gfx_color color);
void gfx_equilateral(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned a, int dir, gfx_color color);

void gfx_rrect(struct gfx_drawable *da, const struct gfx_rect *bb, unsigned r,
    gfx_color color);
void gfx_rrect_xy(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h, unsigned r, gfx_color color);

/*
 * Angle a0 is the start angle, a1 the end angle. They are in degrees, must be
 * in the range 0 - 359, increase clockwise, with 0 degrees in the 12 h
 * position. For a0 = a1. we draw a full circle.
 */

void gfx_arc(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r, 
    unsigned a0, unsigned a1, gfx_color color, gfx_color bg);

/*
 * Power symbol (arc with vertical bar) centered at (x, y), with arc radius r.
 */

void gfx_power_sym(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r,
    unsigned lw, gfx_color color, gfx_color bg);

/*
 * Edit symbol (pencil) inscribed in a square with its upper left corner at
 * (x, y). "width" is the (outer) width of the pen. "length" is the total
 * length, from tip to the flat top.
 *
 * gfx_pencil_sym returns the length of the side of the square.
 */

unsigned gfx_pencil_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned width, unsigned lenght, unsigned lw,
    gfx_color color, gfx_color bg);

/*
 * Gear (setup) symbol. ro and ri are the outer and the inner radius of the
 * circular part of the gear. The teeth are isosceles trapezoids with base
 * width tb, tip width tt, and height of the tip above the circle of th.
 */

void gfx_gear_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned ro, unsigned ri, unsigned tb, unsigned tt, unsigned th,
    gfx_color color, gfx_color bg);

#endif /* !SHAPE_H */
