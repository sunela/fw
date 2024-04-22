/*
 * text.c - Text drawing
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdio.h>
#include <assert.h>

#include "gfx.h"
#include "font.h"
#include "text.h"

#include "mono18.font"
#include "mono24.font"
#include "mono34.font"
#include "mono36.font"
#include "mono58.font"


/* --- Characters ---------------------------------------------------------- */


static inline unsigned char_offset(const struct font *font,
    const struct character *c)
{
	return font->h - (c->h + c->oy);
}


unsigned text_char_size(unsigned *res_w, unsigned *res_h,
    const struct font *font, uint16_t ch)
{
	const struct character *c;

	*res_w = font->w;
	*res_h = font->h;
	c = font_find_char(font, ch);
	assert(c);
	return c->advance;
}


static unsigned text_char_size_offset(unsigned *res_w, unsigned *res_h,
    unsigned *res_oy, const struct font *font, uint16_t ch)
{
	const struct character *c;

	c = font_find_char(font, ch);
	assert(c);
	if (res_w)
		*res_w = c->w + c->ox;
	if (res_h)
		*res_h = c->h;
	if (res_oy)
		*res_oy = char_offset(font, c);
	return c->advance;
}


unsigned text_char(struct gfx_drawable *da, int x1, int y1,
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
	y1 += char_offset(font, c);
//printf("C %x %u x1 %u y1 %u\n", ch, ch, x1, y1);
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
		if (on && more)
			gfx_rect_xy(da, x1 + x, y1 + y, more, 1, color);
		x += more;
		on = !on;
	}
	return c->advance;
}


/* --- Text strings -------------------------------------------------------- */


unsigned text_text_bbox(unsigned x, unsigned y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y, 
    struct gfx_rect *bb)
{
	unsigned next, w, h, oy;
	int headroom = -1;
	bb->x = x;
	bb->y = y;
	bb->w = 0;
	bb->h = 0;

//printf("\"%s\" ", s);
	while (*s) {
		next = text_char_size_offset(&w, &h, &oy, font, *s);
		if (headroom == -1 || (unsigned) headroom > oy)
			headroom = oy;
		h += oy;
		bb->w += s[1] ? next : w;
		if (h > bb->h)
			bb->h = h;
		s++;
	}
//printf(" headroom %d y %u h %u\n", headroom, bb->y, bb->h);
	if (headroom != -1) {
//		bb->y -= headroom;
		bb->h -= headroom;
	}
//printf("\ttotal_w %u max_h %u\n", total_w, max_h);
	if ((align_x & GFX_ALIGN_MASK) == GFX_CENTER) {
		assert(x >= bb->w / 2);
		bb->x = x - bb->w / 2;
	} else if ((align_x & GFX_ALIGN_MASK) == GFX_RIGHT) {
		assert(x >= bb->w);
		bb->x = x - bb->w;
	}

	if ((align_y & GFX_ALIGN_MASK) == GFX_CENTER) {
		assert(y >= bb->h / 2);
		bb->y = y - bb->h / 2;
	} else if ((align_y & GFX_ALIGN_MASK) == GFX_BOTTOM) {
		assert(y >= bb->h);
		bb->y = y - bb->h;
	}
//printf("  y %u h %u\n", bb->y, bb->h);
	return headroom;
}


void text_text(struct gfx_drawable *da, unsigned x, unsigned y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y, gfx_color color)
{
	unsigned headroom;
	struct gfx_rect bb;

	headroom = text_text_bbox(x, y, s, font, align_x, align_y, &bb);
	x = bb.x;
	y = bb.y - headroom;

	while (*s)
{
//printf("\t%u %u '%c'\n", x, y, *s);
		x += text_char(da, x, y, font, *s++, color);
}
}
