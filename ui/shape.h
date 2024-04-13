/*
 * shape.h - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SHAPE_H
#define	SHAPE_H

#include "gfx.h"


void cross(unsigned x, unsigned y, unsigned r, unsigned w,
    gfx_color color);
void equilateral(unsigned x, unsigned y, unsigned a, int dir, gfx_color color);

#endif /* !SHAPE_H */
