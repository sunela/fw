/*
 * ui_list.c - User interface: scrollable list of tappable items
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdlib.h>

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


void *ui_list_pick(const struct ui_list *list, unsigned x, unsigned y)
{
	const struct ui_list_entry *e;
	unsigned pos = list->style->y0;

	if (y < pos)
		return NULL;
	for (e = list->list; e; e = e->next) {
		pos += Y_STEP;
		if (y < pos)
			return e->user;
	}
	return NULL;
}


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

	e = alloc_type(struct ui_list_entry);
	e->first = first;
	e->second = second;
	e->user = user;
	e->next = NULL;
	*list->anchor = e;
	list->anchor = &e->next;
}


void ui_list_end(struct ui_list *list)
{
	const struct ui_list_entry *e;
	const struct ui_list_style *style = list->style;
	unsigned y = list->style->y0;
	unsigned i = 0;

	for (e = list->list; e; e = e->next) {
		unsigned h = 2 * Y_PAD +
		    (e->second ? 2 * Y_STEP + Y_PAD : Y_STEP);

		gfx_rect_xy(&da, 0, y, GFX_WIDTH, h, style->bg[i & 1]);
		if (use_ntext)
			ntext_text(&da, 0, y + Y_PAD + Y_STEP / 2,
			    e->first, &FONT, GFX_LEFT, GFX_CENTER,
			    style->fg[i & 1]);
		else
			gfx_text(&da, 0, y + Y_PAD + Y_STEP / 2,
			    e->first, FONT_SIZE, GFX_LEFT, GFX_CENTER,
			    style->fg[i & 1]);
		if (e->second) {
			if (use_ntext)
				ntext_text(&da, 0, y + 2 * Y_PAD + 1.5 * Y_STEP,
				    e->first, &FONT, GFX_LEFT, GFX_CENTER,
				    style->fg[i & 1]);
			else
				gfx_text(&da, 0, y + 2 * Y_PAD + 1.5 * Y_STEP,
				    e->first, FONT_SIZE, GFX_LEFT, GFX_CENTER,
				    style->fg[i & 1]);
		}
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
