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
#include "debug.h"
#include "alloc.h"
#include "gfx.h"
#include "text.h"
#include "ui.h"
#include "ui_list.h"


#define	DEFAULT_FONT	mono18

#define	Y_PAD		1


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
	return 2 * Y_PAD + (e->second ? 2 * list->text_height + Y_PAD :
	    list->text_height);
}


static const struct font *list_font(const struct ui_list *list)
{
	return list->style->font ? list->style->font : &DEFAULT_FONT;
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
	struct text_query q;

	list->style = style;

	text_query(0, 0, "", list_font(list),
	    GFX_TOP | GFX_MAX, GFX_TOP | GFX_MAX, &q);
	list->text_height = q.h;
	debug("height %d\n", q.h);

	list->list = NULL;
	list->anchor = &list->list;
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
	text_text(&da, 0, y + Y_PAD, e->first, list_font(list),
	    GFX_LEFT, GFX_TOP | GFX_MAX, style->fg[even]);
	if (!e->second)
		return h;
	text_text(&da, 0, y + 2 * Y_PAD + list->text_height, e->second,
	    list_font(list), GFX_LEFT, GFX_TOP | GFX_MAX, style->fg[even]);
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
