/*
 * ui_list.c - User interface: scrollable list of tappable items
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdlib.h>

//#include "hal.h"
#include "lib/alloc.h"
#include "gfx.h"
#include "ui.h"
#include "ui_list.h"


//#define	HOLD_MS	(5 * 1000)

#define	FONT_SIZE	24


struct ui_list_entry {
	const char *label;
	void *user;
	struct ui_list_entry *next;
};


void ui_list_begin(struct ui_list *list)
{
	list->list = NULL;
	list->anchor = &list->list;
}


void ui_list_add(struct ui_list *list, const char *label, void *user)
{
	struct ui_list_entry *e;

	e = alloc_type(struct ui_list_entry);
	e->label = label;
	e->user = user;
	e->next = NULL;
	*list->anchor = e;
	list->anchor = &e->next;
}


void ui_list_end(struct ui_list *list)
{
	const struct ui_list_entry *e;
	unsigned y = 0; // GFX_HEIGHT / 2

	for (e = list->list; e; e = e->next) {
		gfx_text(&da, 0, y, e->label, FONT_SIZE,
		    GFX_LEFT, GFX_TOP, GFX_WHITE);
		y += 24;
	}
}


void ui_list_destroy(struct ui_list *list)
{
	while (list->list) {
		struct ui_list_entry *next = list->list->next;

		free(list->list);
		list->list = next;
	}
}
