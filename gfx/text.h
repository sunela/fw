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

void text_text_bbox(int x, int y, const char *s,
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

#endif /* !TEXT_H */
