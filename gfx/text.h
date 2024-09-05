/*
 * text.h - New text drawing
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef TEXT_H
#define	TEXT_H

#include <stdbool.h>
#include <stdint.h>

#include "gfx.h"
#include "font.h"


struct text_query {
	int x, y, w, h;		/* bounding box */
	int ox, oy;		/* origin (y of baseline) */
	int next;		/* x coordinate of the next character */
};


extern const struct font mono14;
extern const struct font mono18;
extern const struct font mono24;
extern const struct font mono34;
extern const struct font mono36;
extern const struct font mono58;


/*
 * next_char draws a character with the left edge of the "character box" at x,
 * and the baseline at y.
 */

unsigned text_char(struct gfx_drawable *da, int x1, int y1,
    const struct font *font, uint16_t ch, gfx_color color);

void text_query(int x, int y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y,
    struct text_query *q);

void text_bbox(int x, int y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y,
    struct gfx_rect *bb);

/*
 * text_text returns the x position of the next character cell. (If a character
 * is drawn there, its actual leftmost pixel may be at a different place, since
 * it may not begin at the left edge of the character cell.)
 */

unsigned text_text(struct gfx_drawable *da, int x, int y,
    const char *s, const struct font *font, int8_t align_x, int8_t align_y,
    gfx_color color);

/*
 * text_format renders a text such that it fits in a rectangle of width w and
 * height h. It inserts line breaks as needed. Rendering starts at "offset"
 * (vertical) pixels. If the text does not fit in the rectangle, text_format
 * draws as much as it can. It returns for how many pixels the text would
 * extend at the bottom. If the text does not need all the vertical space in
 * rectangle, a negative number is returned.
 *
 * If h is zero, only the size calculations are done, without attempting to
 * render any text.
 */

int text_format(struct gfx_drawable *da, int x, int y, unsigned w, unsigned h,
    unsigned offset, const char *s, const struct font *font, int8_t align_x,
    gfx_color color);

#endif /* !TEXT_H */
