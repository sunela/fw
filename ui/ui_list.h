/*
 * ui_list.h - User interface: scrollable list of tappable items
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_LIST_H
#define	UI_LIST_H

struct ui_list_entry;

struct ui_list {
	struct ui_list_entry *list;
	struct ui_list_entry **anchor;
};


void ui_list_begin(struct ui_list *ctx);
void ui_list_add(struct ui_list *ctx, const char *label, void *user);
void ui_list_end(struct ui_list *ctx);

void ui_list_destroy(struct ui_list *ctx);

#endif /* !UI_LIST_H */
