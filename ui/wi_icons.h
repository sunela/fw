/*
 * wi_icons.h - Widget: array of tappable icons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef WI_ICONS_H
#define	WI_ICONS_H

#include "gfx.h"


struct wi_icons_style {
	unsigned size;		/* button size (square) */
	unsigned button_r;	/* corner radius */
	unsigned gap;		/* horizontal/vertical gap between buttons */
	gfx_color button_fg, button_bg;
};

typedef void (*wi_icons_draw_fn)(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);


/*
 * "icons" is the list of icon-drawing functions. Entries can be NULL, in which
 * case nothing in drawn. If style is NULL, a default style is used.
 *
 * wi_icons_draw operates on an array while wi_icons_draw_access uses an access
 * function, e.g., if the function pointer is stored in a structure containing
 * additional informatiion.
 */

void wi_icons_draw_access(struct gfx_drawable *da, unsigned cx, unsigned cy,
    const struct wi_icons_style *style, 
    wi_icons_draw_fn access(void *user, unsigned i), void *user,
    unsigned n_icons);
void wi_icons_draw(struct gfx_drawable *da, unsigned cx, unsigned cy,
    const struct wi_icons_style *style, const wi_icons_draw_fn *icons,
    unsigned n_icons);

/*
 * wi_icons_select returns -1 if the coordinates are not inside any icon.
 * If style is NULL, a default style is used.
 *
 * Note that the index returned may be that of an entry that was NULL.
 */

int wi_icons_select(unsigned x, unsigned y, unsigned cx, unsigned cy,
    const struct wi_icons_style *style, unsigned n_icons);

#endif /* !WI_ICONS_H */
