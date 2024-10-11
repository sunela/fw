/*
 * ui_overlay.h - User interface: Overlay buttons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_OVERLAY_H
#define	UI_OVERLAY_H

#include "gfx.h"
#include "wi_icons.h"


struct ui_overlay_button {
	void (*draw)(struct gfx_drawable *da,
	    const struct wi_icons_style *style,
	    unsigned x, unsigned y);
	void (*fn)(void *user);
	void *user;
};

struct ui_overlay_params {
	const struct ui_overlay_button *buttons;
	unsigned n_buttons;
	const struct wi_icons_style *style;	/* NULL for default */
};


void ui_overlay_sym_power(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_delete(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_add(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_back(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_next(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_edit(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_setup(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_move_from(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_move_to(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_move_cancel(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_pc_comm(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_folder(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);
void ui_overlay_sym_account(struct gfx_drawable *da,
    const struct wi_icons_style *style, unsigned x, unsigned y);

#endif /* !UI_OVERLAY_H */
