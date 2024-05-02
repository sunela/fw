/*
 * uw_list.c - User interface widget: scrollable list of tappable items
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
#include "uw_list.h"


#define	DEFAULT_FONT	mono18
#define	DEFAULT_OPAD	1
#define	DEFAULT_IPAD	1


struct uw_list_entry {
	const char *first;
	const char *second;
	void *user;
	struct uw_list_entry *next;
};


/* --- Helper functions ---------------------------------------------------- */


static unsigned opad(const struct uw_list *list, const struct uw_list_entry *e)
{
	const struct uw_list_style *s = list->style;

	return s->opad ? s->opad : DEFAULT_OPAD;
}


static unsigned ipad(const struct uw_list *list, const struct uw_list_entry *e)
{
	const struct uw_list_style *s = list->style;

	return s->ipad ? s->ipad : DEFAULT_IPAD;
}


static unsigned entry_height(const struct uw_list *list,
    const struct uw_list_entry *e)
{
	return 2 * opad(list, e) + (e->second ?
	    2 * list->text_height + (ipad(list, e)) : list->text_height);
}


static const struct font *list_font(const struct uw_list *list)
{
	return list->style->font ? list->style->font : &DEFAULT_FONT;
}


/* --- Item selection ------------------------------------------------------ */


struct uw_list_entry *uw_list_pick(const struct uw_list *list,
    unsigned x, unsigned y)
{
	struct uw_list_entry *e;
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


void *uw_list_user(const struct uw_list_entry *entry)
{
	return entry->user;
}


/* --- Render callback ----------------------------------------------------- */


void uw_list_render(struct uw_list *list, struct uw_list_entry *entry)
{
	struct gfx_rect bb = {
		.x = 0,
		.y = list->style->y0,
		.w = GFX_WIDTH,
		.h = entry_height(list, entry),
	};
	const struct uw_list_entry *e;
	bool odd = 0;

	if (!list->style->render)
		return;
	for (e = list->list; e != entry; e = e->next) {
		bb.y += entry_height(list, e);
		odd = !odd;
	}
	list->style->render(list, entry, &bb, odd);
}


/* --- Iterate over all entries -------------------------------------------- */


void uw_list_forall(struct uw_list *list,
    void (*fn)(struct uw_list *list, struct uw_list_entry *entry, void *user),
    void *user)
{
	struct uw_list_entry *e;

	for (e = list->list; e; e = e->next)
		fn(list, e, user);
}


/* --- List construction, display, destruction ----------------------------- */


void uw_list_begin(struct uw_list *list, const struct uw_list_style *style)
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


struct uw_list_entry *uw_list_add(struct uw_list *list,
    const char *first, const char *second, void *user)
{
	struct uw_list_entry *e;

	assert(first);
	e = alloc_type(struct uw_list_entry);
	e->first = first ? stralloc(first) : first;
	e->second = second ? stralloc(second) : second;
	e->user = user;
	e->next = NULL;
	*list->anchor = e;
	list->anchor = &e->next;
	return e;
}


static unsigned draw_entry(const struct uw_list *list,
    const struct uw_list_entry *e, unsigned y, bool odd)
{
	const struct uw_list_style *style = list->style;
	unsigned h = entry_height(list, e);
	struct gfx_rect bb = { .x = 0, .y = y, .w = GFX_WIDTH, .h = h };

	gfx_rect(&da, &bb, style->bg[odd]);
	text_text(&da, 0, y + opad(list, e), e->first, list_font(list),
	    GFX_LEFT, GFX_TOP | GFX_MAX, style->fg[odd]);
	if (e->second)
		text_text(&da, 0,
		    y + opad(list, e) + ipad(list, e) + list->text_height,
		    e->second, list_font(list),
		    GFX_LEFT, GFX_TOP | GFX_MAX, style->fg[odd]);
	if (style->render)
		style->render(list, e, &bb, odd);
	return h;
}


void uw_list_update_entry(struct uw_list *list, struct uw_list_entry *entry,
    const char *first, const char *second, void *user)
{
	bool changed = (first ? !entry->first || strcmp(first, entry->first) :
	    !!entry->first) ||
	    (second ? !entry->second || strcmp(second, entry->second) :
	    !!entry->second);
	const struct uw_list_entry *e;
	unsigned y = list->style->y0;
	int odd = 0;

	entry->user = user;
	if (!changed)
		return;

	free((void *) entry->first);
	free((void *) entry->second);
	entry->first = first ? stralloc(first) : first;
	entry->second = second ? stralloc(second) : second;

	for (e = list->list; e != entry; e = e->next) {
		y += entry_height(list, e);
		odd = !odd;
	}
	draw_entry(list, entry, y, odd);
}


void uw_list_end(struct uw_list *list)
{
	const struct uw_list_entry *e;
	unsigned y = list->style->y0;
	unsigned i = 0;

	for (e = list->list; e; e = e->next) {
		unsigned h;

		h = draw_entry(list, e, y, i & 1);
		i++;
		y += h;
	}
}


void uw_list_destroy(struct uw_list *list)
{
	while (list->list) {
		struct uw_list_entry *next = list->list->next;

		if (list->list->first)
			free((void *) list->list->first);
		if (list->list->second)
			free((void *) list->list->second);
		free(list->list);
		list->list = next;
	}
}
