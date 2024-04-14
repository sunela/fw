/*
 * ntext.c - New text drawing
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <assert.h>

#include "gfx.h"
#include "font.h"
#include "ntext.h"

#include "mono18.font"
#include "mono28.font"
#include "mono34.font"
#include "mono38.font"
#include "mono58.font"


/* --- Characters ---------------------------------------------------------- */


unsigned ntext_char_size(unsigned *res_w, unsigned *res_h,
    const struct font *font, uint16_t ch)
{
	const struct character *c;

	*res_w = font->w;
	*res_h = font->h;
	c = font_find_char(font, ch);
	assert(c);
	return c->advance;
}


unsigned ntext_char(struct gfx_drawable *da, int x1, int y1,
    const struct font *font, uint16_t ch, gfx_color color)
{
	const struct character *c = font_find_char(font, ch);
	const uint8_t *p;
	uint8_t got = 0;
	uint16_t more;
	uint16_t buf = 0;
	bool on;
	unsigned x, y;

//printf("C%x %u\n", ch, ch);
	assert(c);
	if (c->bits == 0)
		return c->advance;
	/* @@@ support bits = 1 later */
	assert(c->bits >= 2);

	x1 += c->ox;
	y1 += c->oy;
	x = 0;
	y = 0;
	p = c->data;
	on = c->start;
	while (x != c->w && y != c->h) {
		more = 0;
		while (1) {
			uint8_t this;

			if (got < c->bits) {
				buf |= *p++ << got;
				got += 8;
			}
			this = (buf & ((1 << c->bits) - 1)) + 1;
			more += this;
			buf >>= c->bits;
			got -= c->bits;
			if (this != 1 << c->bits)
				break;
			more--;
		}
//printf("%u %u %u:%u (w %u)\n", x, y, on, more, c->w);
		while (x + more >= c->w) {
			unsigned d = c->w - x;

			if (on)
				gfx_rect_xy(da, x1 + x, y1 + y, d, 1, color);
			more -= d;
			x = 0;
			y++;
		}	
		if (on)
			gfx_rect_xy(da, x1 + x, y1 + y, more, 1, color);
		x += more;
		on = !on;
	}
	return c->advance;
}


/* --- Text strings -------------------------------------------------------- */


void ntext_text_bbox(unsigned x, unsigned y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y, 
    struct gfx_rect *bb)
{
	unsigned next, w, h;

	bb->x = x;
	bb->y = y;
	bb->w = 0;
	bb->h = 0;

	while (*s) {
		next = ntext_char_size(&w, &h, font, *s);
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


void ntext_text(struct gfx_drawable *da, unsigned x, unsigned y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y, gfx_color color)
{
	if (align_x >= 0 || align_y >= 0) {
		struct gfx_rect bb;

		ntext_text_bbox(x, y, s, font, align_x, align_y, &bb);
		x = bb.x;
		y = bb.y;
	}

	while (*s)
{
//printf("\t%u %u '%c'\n", x, y, *s);
		x += ntext_char(da, x, y, font, *s++, color);
}
}
