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
		r->x = x;
		r->w += r->x - x;
	}
	if (r->y > y) {
		r->y = y;
		r->h += r->y - y;
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


/* https://stackoverflow.com/a/1237519/8179289 */

void gfx_disc(struct gfx_drawable *da, unsigned x, unsigned y, unsigned r, 
    gfx_color color)
{
	gfx_color *p = da->fb + (y - r) * da->w + x - r;
	int dx, dy;

	assert(x >= r && x + r < da->w);
	assert(y >= r && y + r < da->h);
	for (dy = -r; dy <= (int) r; dy++) {
		for (dx = -r; dx <= (int) r; dx++) {
			if (dx * dx + dy * dy <= (int) (r * r))
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
