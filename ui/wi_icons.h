/*
 * wi_icons.h - Widget: array of tappable icons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef WI_ICONS_H
#define	WI_ICONS_H

#include "gfx.h"
#include "ui_overlay.h"

/*
 * @@@ For now, we use "struct ui_overlay_style". Later, we can define the
 * style here, and change the ui_overlay API accordingly.
 */

#include "ui_overlay.h"


typedef void (*wi_icons_draw_fn)(struct gfx_drawable *da,
    const struct ui_overlay_style *style, unsigned x, unsigned y);


/*
 * "icons" is the list of icon-drawing functions. Entries can be NULL, in which
 * case nothing in drawn. If style is NULL, a default style is used.
 */

void wi_icons_draw(struct gfx_drawable *da, unsigned cx, unsigned cy,
    const struct ui_overlay_style *style, const wi_icons_draw_fn *icons,
    unsigned n_icons);

/*
 * wi_icons_select returns -1 if the coordinates are not inside any icon.
 * If style is NULL, a default style is used.
 *
 * Note that the index returned may be that of an entry that was NULL.
 */

int wi_icons_select(unsigned x, unsigned y, unsigned cx, unsigned cy,
    const struct ui_overlay_style *style, unsigned n_icons);

#endif /* !WI_ICONS_H */
