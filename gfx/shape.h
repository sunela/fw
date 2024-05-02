/*
 * shape.h - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SHAPE_H
#define	SHAPE_H

#include "gfx.h"


void gfx_cross(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r,
    unsigned w, gfx_color color);
void gfx_triangle(struct gfx_drawable *da, unsigned x0, unsigned y0,
    unsigned x1, unsigned y1, unsigned x2, unsigned y2, gfx_color color);
void gfx_equilateral(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned a, int dir, gfx_color color);

#endif /* !SHAPE_H */
