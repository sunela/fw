/*
 * gfx/basic.c - Basic drawing operations
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <string.h>
#include <assert.h>

#include "gfx.h"


/* --- Damage rectangle ---------------------------------------------------- */


static void damage(struct gfx_drawable *da, unsigned x, unsigned y,
    unsigned w, unsigned h)
{
	struct gfx_rect *r = &da->damage;

	if (!w || !h)
		return;
	if (!da->changed) {
		da->changed = 1;
		r->x = x;
		r->y = y;
		r->w = w;
		r->h = h;
		return;
	}
	if (r->x > x) {
		r->w += r->x - x;
		r->x = x;
	}
	if (r->y > y) {
		r->h += r->y - y;
		r->y = y;
	}
	if (r->x + r->w < x + w)
		r->w = x + w - r->x;
	if (r->y + r->h < y + h)
		r->h = y + h - r->y;
}


/* --- Filled rectangles --------------------------------------------------- */


void gfx_rect(struct gfx_drawable *da, const struct gfx_rect *r,
    gfx_color color)
{
	gfx_color *p = da->fb + r->y * da->w + r->x;
	unsigned x, y;

	assert(r->x < da->w && r->x + r->w <= da->w);
	assert(r->y < da->h && r->y + r->h <= da->h);
	for (y = 0; y != r->h; y++) {
		/*
		 * @@@ could use memset if all bytes of bg have the same value,
		 * e.g., black or white.
		 */
		for (x = 0; x != r->w; x++)
			*p++ = color;
		p += da->w - r->w;
	}
	damage(da, r->x, r->y, r->w, r->h);
}


void gfx_rect_xy(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h, gfx_color color)
{
	struct gfx_rect r = {
		.x = x,
		.y = y,
		.w = w,
		.h = h,
	};

	gfx_rect(da, &r, color);
}


void gfx_clear(struct gfx_drawable *da, gfx_color bg)
{
	gfx_rect_xy(da, 0, 0, da->w, da->h, bg);
}


/* --- Filled circle (disc) ------------------------------------------------ */


/*
 * https://stackoverflow.com/a/1237519/8179289
 *
 * See also:
 * https://gamedev.stackexchange.com/q/176036
 */

void gfx_disc(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r,
    gfx_color color)
{
	gfx_color *p = da->fb + (y - r) * da->w + x - r;
	int dx, dy;
	int r2 = (r + 0.5) * (r + 0.5);

	assert(x >= r && x + r < da->w);
	assert(y >= r && y + r < da->h);
	for (dy = -r; dy <= (int) r; dy++) {
		for (dx = -r; dx <= (int) r; dx++) {
			if (dx * dx + dy * dy < r2)
				*p = color;
			p++;
		}
		p += da->w - 2 * r - 1;
	}
	damage(da, x - r, y - r, 2 * r + 1, 2 * r + 1);
}


/* --- Compositing --------------------------------------------------------- */


void gfx_copy(struct gfx_drawable *to, unsigned xt, unsigned yt,
    const struct gfx_drawable *from, unsigned xf, unsigned yf,
    unsigned w, unsigned h, int transparent_color)
{
	const gfx_color *src = from->fb + yf * from->w + xf;
	gfx_color *dst = to->fb + yt * to->w + xt;
	unsigned x, y;

	// @@@ we could just copy the changed part
	assert(xf < from->w && xf + w <= from->w);
	assert(yf < from->h && yf + h <= from->h);
	assert(xt < to->w && xt + w <= to->w);
	assert(yt < to->h && yt + h <= to->h);
	for (y = 0; y != h; y++) {
		if (transparent_color < 0) {
			memcpy(dst, src, w * sizeof(gfx_color));
			src += from->w;
			dst += to->w;
		} else {
			for (x = 0; x != w; x++) {
				if (*src != transparent_color)
					*dst = *src;
				src++;
				dst++;
			}
			dst += to->w - w;
		}
	}
	damage(to, xt, yt, w, h);
}


/* --- Scrolling ----------------------------------------------------------- */


/*
 * We scroll the content in the rectangle (x, y) to (x + w - 1, y + h - 1)
 * dx < 0: left
 * dx > 0: right
 * dy < 0: up
 * dy > 0: down
 *
 * The content of the area of dx columns or dy rows that are not overwritten by
 * the scrolling is undefined. E.g., for dx > 0:
 *
 * x,y  x+dx
 *  +----+------------+
 *  | a b c d e f g h |
 *  +-----------------+
 *
 * after scrolling
 *
 *  +----+------------+
 *  | x x a b c d e f |
 *  +-----------------+
 *
 * The undefined area is not (!) marked as damaged. The expected use is to
 * scroll existing content and then to write new content to the then undefined
 * area.
 */


void gfx_hscroll(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h, int dx)
{
	gfx_color *p = da->fb + y * da->w + x;
	unsigned i;

	assert(x + w <= da->w);
	assert(y + h <= da->h);
	assert(dx < (int) w || -dx < (int) w);

	if (!dx)
		return;
	if (dx < 0) {
		unsigned size = (w + dx) * sizeof(gfx_color);

		for (i = 0; i != h; i++) {
			memmove(p, p - dx, size);
			p += da->w;
		}
		damage(da, x, y, w + dx, h);
	} else {
		unsigned size = (w - dx) * sizeof(gfx_color);

		for (i = 0; i != h; i++) {
			memmove(p + dx, p, size);
			p += da->w;
		}
		damage(da, x + dx, y, w - dx, h);
	}
}


void gfx_vscroll(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h, int dy)
{
	gfx_color *a = da->fb + y * da->w + x;
	gfx_color *b = a + (dy < 0 ? -dy : dy) * da->w;
	unsigned i;

	assert(x + w <= da->w);
	assert(y + h <= da->h);
	assert(dy < (int) h && -dy > (int) h);

	if (!dy)
		return;
	if (dy < 0) {
		for (i = -dy; i != h; i++) {
			memcpy(a, b, w * sizeof(gfx_color));
			a += da->w;
			b += da->w;
		}
		damage(da, x, y, w, h + dy);
	} else {
		a += (h - dy) * da->w;
		b += (h - dy) * da->w;
		for (i = dy; i != h; i++) {
			a -= da->w;
			b -= da->w;
			memcpy(b, a, w * sizeof(gfx_color));
		}
		damage(da, x, y + dy, w, h - dy);
	}
}


/* --- Drawable initialization --------------------------------------------- */


void gfx_reset(struct gfx_drawable *da)
{
	da->changed = 0;
}


void gfx_da_init(struct gfx_drawable *da, unsigned w, unsigned h,
    gfx_color *fb)
{
	da->w = w;
	da->h = h;
	da->fb = fb;
	da->changed = 0;
}
