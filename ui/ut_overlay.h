/*
 * ut_overlay.h - User interface tool: Overlay buttons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UT_OVERLAY_H
#define	UT_OVERLAY_H

#include "gfx.h"


struct ut_overlay_params;

struct ut_overlay_button {
	void (*draw)(struct gfx_drawable *tmp_da,
	    const struct ut_overlay_params *params,
	    unsigned x, unsigned y, void *user);
	void (*fn)(void *user);
	void *user;
};

struct ut_overlay_style {
	unsigned size;		/* button size (square) */
	unsigned button_r;	/* corner radius */
	unsigned gap;		/* horizontal/vertical gap between buttons */
	unsigned halo;		/* "halo" around the button array */
	gfx_color button_fg, button_bg;
	gfx_color halo_bg;
};

struct ut_overlay_params {
	const struct ut_overlay_button *buttons;
	unsigned n_buttons;
	const struct ut_overlay_style *style;	/* NULL for default */
};


void ui_overlay_sym_power(struct gfx_drawable *tmp_da,
    const struct ut_overlay_params *params, unsigned x, unsigned y, void *user);

#endif /* !UT_ENTRY_H */
