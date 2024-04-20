/*
 * ui_list.h - User interface: scrollable list of tappable items
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_LIST_H
#define	UI_LIST_H

#include "gfx.h"


struct ui_list_entry;

struct ui_list_style {
	unsigned	y0, y1;
	gfx_color	fg[2];	/* even and odd entries */
	gfx_color	bg[2];	/* even and odd entries */
};

struct ui_list {
	struct ui_list_entry		*list;
	struct ui_list_entry		**anchor;
	const struct ui_list_style	*style;
};


void *ui_list_pick(const struct ui_list *list, unsigned x, unsigned y);

void ui_list_begin(struct ui_list *ctx, const struct ui_list_style *style);
void ui_list_add(struct ui_list *ctx, const char *label, const char *content,
    void *user);
void ui_list_end(struct ui_list *ctx);

void ui_list_destroy(struct ui_list *ctx);

#endif /* !UI_LIST_H */
