/*
 * text.c - Display a text string
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <assert.h>

#include "gfx.h"


void gfx_text_bbox(unsigned x, unsigned y, const char *s,
    unsigned scale, int8_t align_x, int8_t align_y, struct gfx_rect *bb)
{
	unsigned next, w, h;

	bb->x = x;
	bb->y = y;
	bb->w = 0;
	bb->h = 0;

	while (*s) {
		next = gfx_char_size(&w, &h, scale, scale, *s);
		bb->w += s[1] ? next : w;
		if (h > bb->h)
			bb->h = h;
		s++;
	}
//printf("\ttotal_w %u max_h %u\n", total_w, max_h);
	if (align_x == 0) {
		assert(x >= bb->w / 2);
		bb->x = x - bb->w / 2;
	} else if (align_x > 0) {
		assert(x >= bb->w);
		bb->x = x - bb->w;
	}

	if (align_y == 0) {
		assert(y >= bb->h / 2);
		bb->y = y - bb->h/ 2;
	} else if (align_y > 0) {
		assert(y >= bb->h);
		bb->y = y - bb->h;
	}
}


void gfx_text(struct gfx_drawable *da, unsigned x, unsigned y, const char *s,
    unsigned scale, int8_t align_x, int8_t align_y, gfx_color color)
{
	if (align_x >= 0 || align_y >= 0) {
		struct gfx_rect bb;

		gfx_text_bbox(x, y, s, scale, align_x, align_y, &bb);
		x = bb.x;
		y = bb.y;
	}

	while (*s)
{
//printf("\t%u %u '%c'\n", x, y, *s);
		x += gfx_char(da, x, y, scale, scale, *s++, color);
}
}
