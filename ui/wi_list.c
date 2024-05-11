/*
 * wi_list.c - Widget: scrollable list of tappable items
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "alloc.h"
#include "gfx.h"
#include "text.h"
#include "ui.h"
#include "wi_list.h"


#define	DEFAULT_FONT	mono18
#define	DEFAULT_OPAD	1
#define	DEFAULT_IPAD	1

#define	OVER_SCROLL	50	/* to get bottom item out of corner area */


struct wi_list_entry {
	const char *first;
	const char *second;
	void *user;
	struct wi_list_entry *next;
};


/* --- Helper functions ---------------------------------------------------- */


static unsigned opad(const struct wi_list *list, const struct wi_list_entry *e)
{
	const struct wi_list_style *s = list->style;

	return s->opad ? s->opad : DEFAULT_OPAD;
}


static unsigned ipad(const struct wi_list *list, const struct wi_list_entry *e)
{
	const struct wi_list_style *s = list->style;

	return s->ipad ? s->ipad : DEFAULT_IPAD;
}


static unsigned entry_height(const struct wi_list *list,
    const struct wi_list_entry *e)
{
	return 2 * opad(list, e) + (e->second ?
	    2 * list->text_height + (ipad(list, e)) : list->text_height);
}


static const struct font *list_font(const struct wi_list *list)
{
	return list->style->font ? list->style->font : &DEFAULT_FONT;
}


/* --- Item selection ------------------------------------------------------ */


struct wi_list_entry *wi_list_pick(const struct wi_list *list,
    unsigned x, unsigned y)
{
	const struct wi_list_style *style = list->style;
	struct wi_list_entry *e;
	int pos = style->y0 - list->up;

	if ((int) y < pos)
		return NULL;
	for (e = list->list; e; e = e->next) {
		unsigned h = entry_height(list, e);

		pos += style->min_h < h ? h : style->min_h;
		if ((int) y < pos)
			return e;
	}
	return NULL;
}


void *wi_list_user(const struct wi_list_entry *entry)
{
	return entry->user;
}


/* --- Render callback ----------------------------------------------------- */


void wi_list_render_entry(struct wi_list *list, struct wi_list_entry *entry)
{
	struct gfx_rect bb = {
		.x = 0,
		.y = list->style->y0,
		.w = GFX_WIDTH,
		.h = entry_height(list, entry),
	};
	const struct wi_list_style *style = list->style;
	const struct wi_list_entry *e;
	bool odd = 0;

	if (!style->render)
		return;
	if (bb.h < style->min_h)
		bb.h = style->min_h;
	for (e = list->list; e != entry; e = e->next) {
		unsigned h = entry_height(list, e);

		bb.y += style->min_h < h ? h : style->min_h;
		odd = !odd;
	}
	list->style->render(list, entry, &da, &bb, odd);
}


/* --- Iterate over all entries -------------------------------------------- */


void wi_list_forall(struct wi_list *list,
    void (*fn)(struct wi_list *list, struct wi_list_entry *entry, void *user),
    void *user)
{
	struct wi_list_entry *e;

	for (e = list->list; e; e = e->next)
		fn(list, e, user);
}


/* --- List drawing -------------------------------------------------------- */


static void do_draw_entry(const struct wi_list *list,
    const struct wi_list_entry *e, struct gfx_drawable *d,
    const struct gfx_rect *bb, unsigned y, bool odd)
{
	const struct wi_list_style *style = list->style;

	gfx_rect(d, bb, style->bg[odd]);
	text_text(d, 0, y + opad(list, e), e->first, list_font(list),
	    GFX_LEFT, GFX_TOP | GFX_MAX, style->fg[odd]);
	if (e->second)
		text_text(d, 0,
		    y + opad(list, e) + ipad(list, e) + list->text_height,
		    e->second, list_font(list),
		    GFX_LEFT, GFX_TOP | GFX_MAX, style->fg[odd]);
	if (style->render)
		style->render(list, e, d, bb, odd);
}


static unsigned draw_entry(const struct wi_list *list,
    const struct wi_list_entry *e, struct gfx_drawable *d, int y, bool odd)
{
	const struct wi_list_style *style = list->style;
	unsigned h = entry_height(list, e);
	struct gfx_rect bb = { .x = 0, .y = y, .w = GFX_WIDTH, .h = h };
	int top = y; /* avoid going through "unsigned" */

	if (h < style->min_h) {
		y += (style->min_h - h) / 2;
		bb.h = style->min_h;
	}
	assert(bb.h <= style->y1 - style->y0 + 1);

	if (top + (int) bb.h <= (int) style->y0)
		return bb.h;
	if (top > (int) style->y1)
		return bb.h;
	if (top >= (int) style->y0 && top + (int) bb.h - 1 <= (int) style->y1) {
		do_draw_entry(list, e, d, &bb, y, odd);
		return bb.h;
	}

	// @@@ if we allocate from the stack here, the SDK version crashes ...
	// gfx_color tmp_fb[GFX_WIDTH * bb.h];
	static PSRAM gfx_color tmp_fb[GFX_WIDTH * GFX_HEIGHT];
	struct gfx_drawable tmp_da;
	struct gfx_rect tmp_bb = bb;

	tmp_bb.y = 0;
	gfx_da_init(&tmp_da, GFX_WIDTH, bb.h, tmp_fb);
	gfx_clear(&tmp_da, style->bg[odd]); /* better safe than sorry */
	do_draw_entry(list, e, &tmp_da, &tmp_bb, y - bb.y, odd);
	if (top < (int) style->y0)
		gfx_copy(d, 0, style->y0, &tmp_da, 0, style->y0 - top,
		    GFX_WIDTH, bb.h - ((int) style->y0 - top), -1);
	else
		gfx_copy(d, 0, top, &tmp_da, 0, 0,
		    GFX_WIDTH, style->y1 - top + 1, -1);
	return bb.h;
}


static unsigned draw_list(struct wi_list *list)
{
	const struct wi_list_style *style = list->style;
	const struct wi_list_entry *e;
	unsigned y = style->y0;
	unsigned i = 0;

debug("  up %u\n", list->up);
	for (e = list->list; e; e = e->next) {
		unsigned h;

		h = draw_entry(list, e, &da, (int) y - (int) list->up, i & 1);
		i++;
		y += h;
	}

	int ys = (int) y - (int) list->up;

	if (ys >= 0 && ys <= (int) style->y1)
		gfx_rect_xy(&da, 0, ys, GFX_WIDTH, style->y1 - ys + 1,
		    GFX_BLACK);
	return y - style->y0;
}


/* --- Vertical scrolling -------------------------------------------------- */


bool list_scroll(struct wi_list *list, int dy)
{
	const struct wi_list_style *style = list->style;

debug("scrolling %u up %u scroll_from %u dy %d y0 %u y1 %u th %u\n",
    list->scrolling, list->up, list->scroll_from, dy, style->y0, style->y1,
    list->total_height);
	if (dy > 0) {
		if (dy > (int) list->up)
			dy = list->up;
	} else {
		unsigned visible_h =
		    list->total_height + OVER_SCROLL - list->up;
		unsigned win_h = style->y1 - style->y0 + 1;

		if (visible_h < win_h)
			return 0;
		if (visible_h + dy < win_h)
			dy = win_h - visible_h;
	}
	if (!dy)
		return 0;
	list->up -= dy;
	draw_list(list);
	update_display(&da);
	return 1;
}


bool wi_list_moving(struct wi_list *list, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y)
{
	const struct wi_list_style *style = list->style;

	if (from_y < style->y0 || from_y > style->y1)
		return 0;
	if (!list->scrolling) {
		list->scroll_from = list->up;
		list->scrolling = 1;
	}
	if (from_y != to_y)
		list_scroll(list, to_y - from_y);
	return 1;
}


bool wi_list_to(struct wi_list *list, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	bool res;

	if (swipe == us_left || swipe == us_right) {
		wi_list_cancel(list);
		return 0;
	}
	res = wi_list_moving(list, from_x, from_y, to_x, to_y);
	list->scrolling = 0;
	return res;
}


void wi_list_cancel(struct wi_list *list)
{
debug("wi_list_cancel\n");
	if (list->scrolling) {
		if (list->scroll_from != list->up) {
			list->up = list->scroll_from;
			draw_list(list);
			update_display(&da);
		}
		list->scrolling = 0;
	}
}


/* --- List construction, update, destruction ------------------------------ */


void wi_list_begin(struct wi_list *list, const struct wi_list_style *style)
{
	struct text_query q;

	list->style = style;

	text_query(0, 0, "", list_font(list),
	    GFX_TOP | GFX_MAX, GFX_TOP | GFX_MAX, &q);
	list->text_height = q.h;
	debug("height %d\n", q.h);

	list->list = NULL;
	list->anchor = &list->list;
	list->total_height = 0;
	list->scrolling = 0;
}


struct wi_list_entry *wi_list_add(struct wi_list *list,
    const char *first, const char *second, void *user)
{
	struct wi_list_entry *e;

	assert(first);
	e = alloc_type(struct wi_list_entry);
	e->first = first ? stralloc(first) : first;
	e->second = second ? stralloc(second) : second;
	e->user = user;
	e->next = NULL;
	*list->anchor = e;
	list->anchor = &e->next;
	return e;
}


void wi_list_update_entry(struct wi_list *list, struct wi_list_entry *entry,
    const char *first, const char *second, void *user)
{
	const struct wi_list_style *style = list->style;
	bool changed = (first ? !entry->first || strcmp(first, entry->first) :
	    !!entry->first) ||
	    (second ? !entry->second || strcmp(second, entry->second) :
	    !!entry->second);
	const struct wi_list_entry *e;
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
		unsigned h = entry_height(list, e);

		y += h < style->min_h ? style->min_h : h;
		odd = !odd;
	}
	draw_entry(list, entry, &da, y, odd);
}


void wi_list_end(struct wi_list *list)
{
	list->total_height = draw_list(list);
}


void wi_list_destroy(struct wi_list *list)
{
	while (list->list) {
		struct wi_list_entry *next = list->list->next;

		if (list->list->first)
			free((void *) list->list->first);
		if (list->list->second)
			free((void *) list->list->second);
		free(list->list);
		list->list = next;
	}
}
