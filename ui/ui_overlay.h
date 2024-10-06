/*
 * ui_overlay.h - User interface: Overlay buttons
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_OVERLAY_H
#define	UI_OVERLAY_H

#include "gfx.h"


struct ui_overlay_params;

struct ui_overlay_button {
	void (*draw)(struct gfx_drawable *tmp_da,
	    const struct ui_overlay_params *params,
	    unsigned x, unsigned y, void *user);
	void (*fn)(void *user);
	void *user;
};

struct ui_overlay_style {
	unsigned size;		/* button size (square) */
	unsigned button_r;	/* corner radius */
	unsigned gap;		/* horizontal/vertical gap between buttons */
	gfx_color button_fg, button_bg;
};

struct ui_overlay_params {
	const struct ui_overlay_button *buttons;
	unsigned n_buttons;
	const struct ui_overlay_style *style;	/* NULL for default */
};


bool button_in(unsigned cx, unsigned cy, unsigned, unsigned y);
void button_draw_add(unsigned x, unsigned y);

void ui_overlay_sym_power(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_delete(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_add(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_back(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_next(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_edit(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_setup(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_move_from(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_move_to(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_move_cancel(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_pc_comm(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);
void ui_overlay_sym_folder(struct gfx_drawable *da,
    const struct ui_overlay_params *params, unsigned x, unsigned y, void *user);

#endif /* !UI_ENTRY_H */
