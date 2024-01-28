/*
 * gfx/long_text.c - Long text, with horizontal scrolling
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <assert.h>

#include "gfx.h"
#include "long_text.h"


static void prepare(struct long_text *lt, const char *s, unsigned scale,
    gfx_color color, gfx_color bg)
{
	struct gfx_rect bb;

	gfx_text_bbox(0, 0, s, scale, GFX_LEFT, GFX_TOP, &bb);
	assert(bb.w <= MAX_W);
	assert(bb.h <= MAX_H);

	gfx_da_init(&lt->buf, bb.w, bb.h, lt->fb);
	gfx_clear(&lt->buf, bg);
	gfx_text(&lt->buf, 0, 0, s, scale, GFX_LEFT, GFX_TOP, color);
}


static void draw(struct long_text *lt, struct gfx_drawable *da,
    unsigned x, int y, unsigned w, unsigned h)
{
	unsigned y0; /* y start in destination drawable */
	unsigned yb; /* y start in buffer */

	if (y >= 0) {
		y0 = y;
		yb = 0;
	} else {
		y0 = 0;
		yb = -y;
		h += y;
	}
	if (y0 + h > da->h)
		h = da->h - y0;
	if (w >= lt->buf.w) {
		gfx_copy(da, x, y0, &lt->buf, 0, yb, lt->buf.w, h, -1);
		gfx_rect_xy(da, x + lt->buf.w, y0, w - lt->buf.w, lt->buf.h,
		    lt->bg);
	} else {
		gfx_copy(da, x, y, &lt->buf, 0, 0, w, lt->buf.h, -1);
	}
// @@@ should we assume the background has been cleared ?
// note: the gfx_rect_xy above would still be needed in this case, since it
// also clears residues from previous scrolled text.
	if (lt->buf.h != h)
		gfx_rect_xy(da, x, y + lt->buf.h, w, h - lt->buf.h,
		    lt->bg);
}


void long_text_setup(struct long_text *lt, struct gfx_drawable *da,
    unsigned x, int y, unsigned w, unsigned h, const char *s, unsigned scale,
    gfx_color color, gfx_color bg)
{
	assert(x < da->w);
	assert(x + w <= da->h);
	assert(y + (int) h >= 0);
	assert(y < (int) da->h);
	assert(y + (int) h <= (int) da->h);

	lt->x = x;
	lt->y = y;
	lt->w = w;
	lt->h = h;
	lt->bg = bg;
	lt->offset = 0;

	prepare(lt, s, scale, color, bg);

	/* we only need to draw the text once */
	if (w >= lt->buf.w)
		draw(lt, da, x, y, w, h);
}


bool long_text_scroll(struct long_text *lt, struct gfx_drawable *da, int dx)
{
	unsigned offset = lt->offset;
	unsigned y0; /* y start in destination drawable */
	unsigned yb; /* y start in buffer */
	unsigned h; /* height of area actually drawn */

	if (lt->w >= lt->buf.w)
		return 0;
	assert(dx < 0);
	assert(dx <= (int) lt->w);

	/*
	 * @@@ should we do circular scrolling ?
	 */
	if (offset + lt->w <= lt->buf.w + dx) {
		lt->offset = offset - dx;
	} else {
		dx = -(lt->buf.w - lt->w - offset);
		offset = lt->buf.w - lt->w + dx;
		lt->offset = 0;
	}

	if (!offset) {
		draw(lt, da, lt->x, lt->y, lt->w, lt->h);
		return 1;
	}

	if (lt->y >= 0) {
		y0 = lt->y;
		yb = 0;
		h = lt->buf.h;
	} else {
		y0 = 0;
		yb = -lt->y;
		h = lt->buf.h + lt->y;
	}
	if (y0 + h > da->h)
		h = da->h - y0;

	gfx_hscroll(da, lt->x, y0, lt->w, lt->buf.h, dx);
	gfx_copy(da, lt->x + lt->w + dx, y0, &lt->buf, lt->w + offset, yb,
	    -dx, lt->buf.h, -1);

	return !offset || !lt->offset;
}
