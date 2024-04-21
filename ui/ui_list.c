/*
 * ui_list.c - User interface: scrollable list of tappable items
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "lib/alloc.h"
#include "gfx.h"
#include "ntext.h"
#include "ui.h"
#include "ui_list.h"


/* @@@ move font, etc. to style after we drop the vector fonts */

#define	FONT_SIZE	24
#define	FONT		mono18

#define	Y_PAD		1
#define	Y_STEP		(FONT_SIZE + 2 * Y_PAD)


struct ui_list_entry {
	const char *first;
	const char *second;
	void *user;
	struct ui_list_entry *next;
};


/* --- Helper functions ---------------------------------------------------- */


static unsigned entry_height(const struct ui_list *list,
    const struct ui_list_entry *e)
{
	return 2 * Y_PAD + (e->second ? 2 * Y_STEP + Y_PAD : Y_STEP);
}


/* --- Item selection ------------------------------------------------------ */


struct ui_list_entry *ui_list_pick(const struct ui_list *list,
    unsigned x, unsigned y)
{
	struct ui_list_entry *e;
	unsigned pos = list->style->y0;

	if (y < pos)
		return NULL;
	for (e = list->list; e; e = e->next) {
		pos += entry_height(list, e);
		if (y < pos)
			return e;
	}
	return NULL;
}


void *ui_list_user(const struct ui_list_entry *entry)
{
	return entry->user;
}


/* --- List construction, display, destruction ----------------------------- */


void ui_list_begin(struct ui_list *list, const struct ui_list_style *style)
{
	list->list = NULL;
	list->anchor = &list->list;
	list->style = style;
}


void ui_list_add(struct ui_list *list, const char *first, const char *second,
    void *user)
{
	struct ui_list_entry *e;

	assert(first);
	e = alloc_type(struct ui_list_entry);
	e->first = first;
	e->second = second;
	e->user = user;
	e->next = NULL;
	*list->anchor = e;
	list->anchor = &e->next;
}


static unsigned draw_entry(const struct ui_list *list,
    const struct ui_list_entry *e, unsigned y, bool even)
{
	const struct ui_list_style *style = list->style;
	unsigned h = entry_height(list, e);

	gfx_rect_xy(&da, 0, y, GFX_WIDTH, h, style->bg[even]);
	if (use_ntext)
		ntext_text(&da, 0, y + Y_PAD + Y_STEP / 2,
		    e->first, &FONT, GFX_LEFT, GFX_CENTER,
		    style->fg[even]);
	else
		gfx_text(&da, 0, y + Y_PAD + Y_STEP / 2,
		    e->first, FONT_SIZE, GFX_LEFT, GFX_CENTER,
		    style->fg[even]);
	if (!e->second)
		return h;
	if (use_ntext)
		ntext_text(&da, 0, y + 2 * Y_PAD + 1.5 * Y_STEP,
		    e->second, &FONT, GFX_LEFT, GFX_CENTER,
		    style->fg[even]);
	else
		gfx_text(&da, 0, y + 2 * Y_PAD + 1.5 * Y_STEP,
		    e->second, FONT_SIZE, GFX_LEFT, GFX_CENTER,
		    style->fg[even]);
	return h;
}


void ui_list_update_entry(struct ui_list *list, struct ui_list_entry *entry,
    const char *first, const char *second, void *user)
{
	bool changed = (first ? !entry->first || strcmp(first, entry->first) :
	    !!entry->first) ||
	    (second ? !entry->second || strcmp(second, entry->second) :
	    !!entry->second);
	const struct ui_list_entry *e;
	unsigned y = list->style->y0;
	int even = 0;

	entry->first = first;
	entry->second = second;
	entry->user = user;
	if (!changed)
		return;
	
	for (e = list->list; e != entry; e = e->next) {
		y += entry_height(list, e);
		even = !even;
	}
	draw_entry(list, entry, y, even);
}


void ui_list_end(struct ui_list *list)
{
	const struct ui_list_entry *e;
	unsigned y = list->style->y0;
	unsigned i = 0;

	for (e = list->list; e; e = e->next) {
		unsigned h;

		h = draw_entry(list, e, y, i & 1);
		i++;
		y += h;
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
