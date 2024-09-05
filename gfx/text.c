/*
 * text.c - Text drawing
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "debug.h"
#include "alloc.h"
#include "gfx.h"
#include "font.h"
#include "text.h"

//#define DEBUG

#include "mono14.font"
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

//debug("C%x %u\n", ch, ch);
	assert(c);
	if (c->bits == 0)
		return c->advance;

	x1 += c->ox;
	y1 -= c->oy + c->h - 1;
	x = 0;
	y = 0;
	p = c->data;
	on = c->start;
	while (x != c->w && y != c->h) {
		if (c->bits == 1) {
			if (!got) {
				buf = *p++;
				got = 8;
			}
			on = buf & 1;
			buf >>= 1;
			got--;
			more = 1;
		} else {
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
		}
//debug("%u %u %u:%u (w %u)\n", x, y, on, more, c->w);
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
			debug("character '%c' not found\n", ch);
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
	unsigned max_adv = 0;

	if (len) {
		const struct character *c;

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
	a->next = font->ox + max_adv * len;
}


static void text_align(struct text_query *q, const struct text_area *a,
    int x, int y, int8_t align_x, int8_t align_y)
{
#ifdef DEBUG
	debug("A ll %d %d ur %d %d\n", a->x0, a->y0, a->x1, a->y1);
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
	debug("Q x %d y %d w %d h %d ox %d oy %d next %d\n",
	    q->x, q->y, q->w, q->h, q->ox, q->oy, q->next);
#endif
}


static void query_rendered(struct text_query *q, int x, int y,
    const char *s, const struct font *font, int8_t align_x, int8_t align_y)
{
	struct text_area a;

	area_rendered(&a, s, font);
	text_align(q, &a, x, y, align_x, align_y);
}


static void query_max(struct text_query *q, int x, int y,
    unsigned len, const struct font *font, int8_t align_x, int8_t align_y)
{
	struct text_area a;

	area_max(&a, len, font);
	text_align(q, &a, x, y, align_x, align_y);
}


void text_query(int x, int y, const char *s,
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


void text_bbox(int x, int y, const char *s,
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


unsigned text_text(struct gfx_drawable *da, int x, int y,
    const char *s, const struct font *font, int8_t align_x, int8_t align_y,
    gfx_color color)
{
	struct text_query q;

	text_query(x, y, s, font, align_x, align_y, &q);
	while (*s)
		q.ox += text_char(da, q.ox, q.oy, font, *s++, color);
	return q.ox;
}


/* --- Format text with line breaks ---------------------------------------- */


static int any_char(int ch)
{
	return 1;
}


static char *find_break(char *s, unsigned w, const struct font *font,
    int (*may_break)(int ch))
{
	char *last = NULL;
	char *end = s;

	while (1) {
		struct text_query q;
		char ch;

		if (!*end)
			return last;
		// break before or after breakable
		if (last == end && may_break(*end)) {
			end++;
		} else {
			do end++;
			while (*end && !may_break(*end));
		}
		ch = *end;
		*end = 0;
		text_query(0, 0, s, font, GFX_LEFT, GFX_TOP, &q);
		*end = ch;
		if (q.w > (int) w)
			return last == s ? NULL : last;
		last = end;
	}
}


int text_format(struct gfx_drawable *da, int x, int y, unsigned w, unsigned h,
    unsigned offset, const char *s, const struct font *font,
    int8_t align_x, gfx_color color)
{
	char *tmp = stralloc(s);
	char *p = tmp;
	int pos = -offset;

	while (1) {
		char *end, ch;
		struct text_query q;

		/* @@@ work around bug in SDK gcc/libc */
		while (*p && isspace((int) *p))
			p++;
		if (!*p)
			break;
		end = find_break(p, w, font, isspace);
		if (!end)
			end = find_break(p, w, font, ispunct);
		if (!end) {
			end = find_break(p, w, font, any_char);
			assert(end);
		}
		ch = *end;
		*end = 0;
		text_query(0, 0, p, font, GFX_LEFT, GFX_TOP | GFX_MAX, &q);
		if (pos + q.h >= 0 && pos < (int) h) {
			unsigned x0;

			switch (align_x) {
			case GFX_LEFT:
				x0 = x;
				break;
			case GFX_RIGHT:
				x0 = x + (w - q.w);
				break;
			case GFX_CENTER:
				x0 = x + (w - q.w) / 2;
				break;
			default:
				abort();
			}
			gfx_clip_xy(da, x, y, w, h);
			text_text(da, x0, y + pos + q.oy, p, font,
			    GFX_LEFT, GFX_ORIGIN, color);
			gfx_clip(da, NULL);
		}
		pos += q.h;
		*end = ch;
		if (!ch)
			break;
		p = end;
	}
	free(tmp);
	return pos - h;
}
