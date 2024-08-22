/*
 * shape.h - Common shapes
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SHAPE_H
#define	SHAPE_H

#include "gfx.h"


/* --- Simple shapes ------------------------------------------------------- */


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

void gfx_arc(struct gfx_drawable *da, int x, int y, unsigned r, 
    unsigned a0, unsigned a1, gfx_color color, gfx_color bg);


/* --- Symbols ------------------------------------------------------------- */


/*
 * Power symbol (arc with vertical bar) centered at (x, y), with arc radius r.
 *
 * gfx_power_sym returns the distance between the top of the vertical bar and
 * the arc center, which is greater than the radius of the arc. da can be set
 * to zero if we are only interested in the return value.
 */

unsigned gfx_power_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned r, unsigned lw, gfx_color color, gfx_color bg);

/*
 * Edit symbol (pencil) inscribed in a square with its upper left corner at
 * (x, y). "width" is the (outer) width of the pen. "length" is the total
 * length, from tip to the flat top.
 *
 * gfx_pencil_sym returns the length of the side of the square. "da" can be set
 * to NULL if we are only interested in the return value.
 */

unsigned gfx_pencil_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned width, unsigned length, unsigned lw,
    gfx_color color, gfx_color bg);

/*
 * Gear (setup) symbol. ro and ri are the outer and the inner radius of the
 * circular part of the gear. The teeth are isosceles trapezoids with base
 * width tb, tip width tt, and height of the tip above the circle of th.
 */

void gfx_gear_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned ro, unsigned ri, unsigned tb, unsigned tt, unsigned th,
    gfx_color color, gfx_color bg);

/*
 * Entry movement symbol. The symbol consists of two boxes, left and right,
 * with an arrowed arc goin from left to right. If "from" is true, the left
 * box is filled, else it is empty. If "to" is 0 or 1, the right box is drawn
 * accordingly. If "to" is negative, the arrowhead and the right box are
 * omitted, and a diagonal cross is drawn instead on the right side, indicating
 * cancellation of the operation.
 *
 * The symbol is centered at (x, y).
 *
 * box_size is the outer size of each box. box_ro is the corner radius "lw" is
 * the line width, used for empty boxes, the arc, the space between boxes, the
 * space between boxes and arc, and the cross.
 */

void gfx_move_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned box_size, unsigned box_ro, unsigned lw,
    bool from, int to, gfx_color color, gfx_color bg);

void gfx_pc_comm_sym(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned side, gfx_color color, gfx_color bg);


/* --- Other UI items ------------------------------------------------------ */


/*
 * Checkbox. Draws a square box centered at (x, y) with outer width w (pixels)
 * and linewidth lw. The inside is cleared in the background color. If "on" is
 * set, a diagonal cross is then drawn inside the box, also using lw.
 */

void gfx_checkbox(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned w, unsigned lw, bool on, gfx_color color, gfx_color bg);

#endif /* !SHAPE_H */
