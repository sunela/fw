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


static void damage(struct gfx_drawable *da, int x, int y, int w, int h)
{
	struct gfx_rect *r = &da->damage;

	assert(x >= 0 && y >= 0);
	assert(w >= 0 && h >= 0);
	assert(x + w <= (int) da->w && y + h <= (int) da->h);
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


/* --- Clipping ------------------------------------------------------------ */


void gfx_clip(struct gfx_drawable *da, const struct gfx_rect *clip)
{
	if (clip) {
		assert(!da->clipping);
		assert(clip->x >= 0);
		assert(clip->y >= 0);
		assert(clip->x + clip->w <= (int) da->w);
		assert(clip->y + clip->h <= (int) da->h);
		da->clip = *clip;
	} else {
		assert(da->clipping);
	}
	da->clipping = clip;
}


void gfx_clip_xy(struct gfx_drawable *da, unsigned x, unsigned y, unsigned w,
    unsigned h)
{
	struct gfx_rect clip = {
		.x	= x,
		.y	= y,
		.w	= w,
		.h	= h,
	};

	gfx_clip(da, &clip);
}


/* --- Filled rectangles --------------------------------------------------- */


void gfx_rect_xy(struct gfx_drawable *da, int x, int y, int w, int h,
    gfx_color color)
{
	gfx_color *p;
	int ix, iy;

	if (w <= 0 || h <= 0)
		return;
	if (da->clipping) {
		if (x >= da->clip.x + da->clip.w)
			return;
		if (y >= da->clip.y + da->clip.h)
			return;
		if (x + w <= da->clip.x)
			return;
		if (y + h <= da->clip.y)
			return;
		if (x < da->clip.x) {
			w -= da->clip.x - x;
			x = da->clip.x;
		}
		if (y < da->clip.y) {
			h -= da->clip.y - y;
			y = da->clip.y;
		}
		if (x + w > da->clip.x + da->clip.w)
			w = da->clip.x + da->clip.w - x;
		if (y + h > da->clip.y + da->clip.h)
			h = da->clip.y + da->clip.h - y;
	}

	assert(w >= 0);
	assert(h >= 0);
	assert(x < (int) da->w);
	assert(w <= (int) da->w);
	assert(x + w <= (int) da->w);
	assert(y < (int) da->h);
	assert(h <= (int) da->h);
	assert(y + h <= (int) da->h);

	p = da->fb + y * da->w + x;
	for (iy = 0; iy != h; iy++) {
		/*
		 * @@@ could use memset if all bytes of bg have the same value,
		 * e.g., black or white.
		 */
		for (ix = 0; ix != w; ix++)
			*p++ = color;
		p += da->w - w;
	}
	damage(da, x, y, w, h);
}


void gfx_rect(struct gfx_drawable *da, const struct gfx_rect *bb,
    gfx_color color)
{
	gfx_rect_xy(da, bb->x, bb->y, bb->w, bb->h, color);
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

void gfx_disc(struct gfx_drawable *da, int x, int y, unsigned r,
    gfx_color color)
{
	gfx_color *p = da->fb + (y - r) * (int) da->w + x - r;
	int dx, dy;
	int r2 = (r + 0.5) * (r + 0.5);

	if (!da->clipping) {
		assert(x >= (int) r);
		assert(x + (int) r < (int) da->w);
		assert(y >= (int) r);
		assert(y + (int) r < (int) da->h);
	}
	for (dy = -r; dy <= (int) r; dy++) {
		if (!da->clipping ||
		    (dy >= da->clip.y && dy < da->clip.y + da->clip.h))
			for (dx = -r; dx <= (int) r; dx++) {
				if (!da->clipping ||
		    		    (dx >= da->clip.x &&
				    dx < da->clip.x + da->clip.w))
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
	da->clipping = 0;
}
