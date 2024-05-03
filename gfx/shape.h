/*
 * shape.h - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SHAPE_H
#define	SHAPE_H

#include "gfx.h"


void gfx_diagonal_cross(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned r, unsigned w, gfx_color color);
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

void gfx_power_sym(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r,
    unsigned lw, gfx_color color, gfx_color bg);

#endif /* !SHAPE_H */
