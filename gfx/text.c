/*
 * text.c - Display a text string
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <assert.h>

#include "gfx.h"


void gfx_text(struct gfx_drawable *da, unsigned x, unsigned y, const char *s,
    unsigned scale, int8_t align_x, int8_t align_y, gfx_color color)
{
	if (align_x >= 0 || align_y >= 0) {
		unsigned total_w = 0;
		unsigned max_h = 0;
		const char *t;
		unsigned next, w, h;

		for (t = s; *t; t++) {
			next = gfx_char_size(&w, &h, scale, scale, *t);
			total_w += t[1] ? next : w;
			if (h > max_h)
				max_h = h;
		}
//printf("\ttotal_w %u max_h %u\n", total_w, max_h);
		if (align_x == 0) {
			assert(x >= total_w / 2);
			x -= total_w / 2;
		} else if (align_x > 0) {
			assert(x >= total_w);
			x -= total_w;
		}

		if (align_y == 0) {
			assert(y >= max_h / 2);
			y -= max_h / 2;
		} else if (align_y > 0) {
			assert(y >= max_h);
			y -= max_h;
		}
	}

	while (*s)
{
//printf("\t%u %u '%c'\n", x, y, *s);
		x += gfx_char(da, x, y, scale, scale, *s++, color);
}
}
