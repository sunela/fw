/*
 * gfx/long_text.h - Long text, with horizontal scrolling
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ If we scroll vertically, this API requires a reset of horizontal
 * scrolling, i.e., a full redraw, since we can't adjust the viewport.
 * Is this good or bad ?
 */

#ifndef LONG_TEXT_H
#define	LONG_TEXT_H

#include <stdbool.h>

#include "gfx.h"
#include "font.h"


#define	MAX_W	1000
#define	MAX_H	50


struct long_text {
	struct gfx_drawable	buf;
	gfx_color		fb[MAX_W * MAX_H];
	unsigned		x;	/* viewport */
	int			y;	/* can be negative */
	unsigned		w, h;
	gfx_color		bg;
	unsigned		offset;
};


/*
 * Note that "y" is signed and can be negative, e.g., to show text that has
 * partially scrolled off. Likewise, "h" can be less than the text height.
 */

void long_text_setup(struct long_text *lt, struct gfx_drawable *da,
    unsigned x, int y, unsigned w, unsigned h, const char *s,
    const struct font *font, gfx_color color, gfx_color bg);

/*
 * long_text_scroll returns 1 if we're at the beginning or the end of the text
 * and should wait a little longer.
 */

bool long_text_scroll(struct long_text *lt, struct gfx_drawable *da, int dx);

#endif /* !LONG_TEXT_H */
