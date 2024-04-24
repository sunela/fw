/*
 * text.c - Text drawing
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "gfx.h"
#include "font.h"
#include "text.h"


#include "mono18.font"
#include "mono24.font"
#include "mono34.font"
#include "mono36.font"
#include "mono58.font"


struct text_area {
	int x0, x1, y0, y1; /* bbox for origin (0, 0), with Y+ pointing up */
	int next;	/* x position of next character */
};


/* --- Characters ---------------------------------------------------------- */


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
	y1 -= c->oy + c->h - 1;
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


/* --- Bounding boxes and alignment ---------------------------------------- */


static void area_rendered(struct text_area *a, const char *s,
    const struct font *font)
{
	bool first = 1;

	a->x0 = a->x1 = a->y0 = a->y1 = a->next = 0;
	while (*s) {
		char ch = *s++;
		const struct character *c = font_find_char(font, ch);

		if (!c) {
			fprintf(stderr, "character '%c' not found\n", ch);
			continue;
		}
		if (first) {
			a->x0 = c->ox;
			a->next = 0;
		}
		a->x1 = a->next + c->ox + c->w - 1;
		if (first || c->oy + c->h - 1 > a->y1)
			a->y1 = c->oy + c->h - 1;
		if (first || c->oy < a->y0)
			a->y0 = c->oy;
		a->next += c->advance;
		first = 0;
	}
}


static void area_max(struct text_area *a, unsigned len,
    const struct font *font)
{
	if (len) {
		const struct character *c;
		unsigned max_adv = 0;

		/* @@@ we should do that at build time */
		for (c = font->chars; c != font->chars + font->n_chars; c++)
			if (c->advance > max_adv)
				max_adv = c->advance;
		a->x0 = font->ox;
		a->x1 = font->ox + max_adv * (len - 1) + font->w - 1;
	} else {
		a->x0 = a->x1 = a->next = 0;
	}
	a->y0 = font->oy;
	a->y1 = font->oy + font->h - 1;
}


static void text_align(struct text_query *q, const struct text_area *a,
    unsigned x, unsigned y, int8_t align_x, int8_t align_y)
{
#ifdef DEBUG
	printf("A ll %d %d ur %d %d\n", a->x0, a->y0, a->x1, a->y1);
#endif
	if (a->next || (align_y & GFX_MAX)) {
		q->w = a->x1 - a->x0 + 1;
		q->h = a->y1 - a->y0 + 1;
	} else {
		q->w = 0;
		q->h = 0;
	}
	switch (align_x & GFX_ALIGN_MASK) {
	case GFX_ORIGIN:
		q->x = x + a->x0;
		q->ox = x;
		break;
	case GFX_LEFT:
		q->x = x;
		q->ox = x - a->x0;
		break;
	case GFX_CENTER:
		q->x = x - q->w / 2;
		q->ox = x - q->w / 2 - a->x0;
		break;
	case GFX_RIGHT:
		q->x = q->w ? x - q->w + 1 : x;
		q->ox = x - (q->w ? q->w - 1 : 0) - a->x0;
		break;
	default:
		abort();
	}
	switch (align_y & GFX_ALIGN_MASK) {
	case GFX_ORIGIN:
		q->y = y - a->y1;
		q->oy = y;
		break;
	case GFX_TOP:
		q->y = y;
		q->oy = y + a->y1;
		break;
	case GFX_CENTER:
		q->y = y - q->h / 2;
		q->oy = y - q->h / 2 + a->y1;
		break;
	case GFX_BOTTOM:
		q->y = q->h ? y - q->h + 1 : y;
		q->oy = y - (q->h ? q->h - 1 : 0) + a->y1;
		break;
	default:
		abort();
	}
	q->next = q->ox + a->next;
#ifdef DEBUG
	printf("Q x %d y %d w %d h %d ox %d oy %d next %d\n",
	    q->x, q->y, q->w, q->h, q->ox, q->oy, q->next);
#endif
}


static void query_rendered(struct text_query *q, unsigned x, unsigned y,
    const char *s, const struct font *font, int8_t align_x, int8_t align_y)
{
	struct text_area a;

	area_rendered(&a, s, font);
	text_align(q, &a, x, y, align_x, align_y);
}


static void query_max(struct text_query *q, unsigned x, unsigned y,
    unsigned len, const struct font *font, int8_t align_x, int8_t align_y)
{
	struct text_area a;

	area_max(&a, len, font);
	text_align(q, &a, x, y, align_x, align_y);
}


void text_query(unsigned x, unsigned y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y, 
    struct text_query *q)
{
	struct text_query q_text, q_max;

	if (!((align_x & align_y) & GFX_MAX))
		query_rendered(&q_text, x, y, s, font, align_x, align_y);
	if ((align_x | align_y) & GFX_MAX)
		query_max(&q_max, x, y, strlen(s), font, align_x, align_y);
	if (align_x & GFX_MAX) {
		q->x = q_max.x;
		q->w = q_max.w;
		q->ox = q_max.ox;
		q->next  = q_max.next;
	} else {
		q->x = q_text.x;
		q->w = q_text.w;
		q->ox = q_text.ox;
		q->next  = q_text.next;
	}
	if (align_y & GFX_MAX) {
		q->y = q_max.y;
		q->h = q_max.h;
		q->oy = q_max.oy;
	} else {
		q->y = q_text.y;
		q->h = q_text.h;
		q->oy = q_text.oy;
	}
}


void text_text_bbox(unsigned x, unsigned y, const char *s,
    const struct font *font, int8_t align_x, int8_t align_y, 
    struct gfx_rect *bb)
{
	struct text_query q;

	text_query(x, y, s, font, align_x, align_y, &q);
	bb->x = q.x;
	bb->y = q.y;
	bb->w = q.w;
	bb->h = q.h;
}


/* --- Align and render text string ---------------------------------------- */


unsigned text_text(struct gfx_drawable *da, unsigned x, unsigned y,
    const char *s, const struct font *font, int8_t align_x, int8_t align_y,
    gfx_color color)
{
	struct text_query q;

	text_query(x, y, s, font, align_x, align_y, &q);
	while (*s)
		q.ox += text_char(da, q.ox, q.oy, font, *s++, color);
	return q.ox;
}
