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


extern const struct font mono18;
extern const struct font mono24;
extern const struct font mono34;
extern const struct font mono36;
extern const struct font mono58;


unsigned text_char_size(unsigned *res_w, unsigned *res_h,
    const struct font *font, uint16_t ch);
unsigned text_char(struct gfx_drawable *da, int x1, int y1,
    const struct font *font, uint16_t ch, gfx_color color);

unsigned text_text_bbox(unsigned x, unsigned y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y,
    struct gfx_rect *bb);
void text_text(struct gfx_drawable *da, unsigned x, unsigned y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y, gfx_color color);

#endif /* !TEXT_H */
