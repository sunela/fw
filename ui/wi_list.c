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
	const struct wi_list_style *style = list->style;

	return style->opad ? style->opad : DEFAULT_OPAD;
}


static unsigned ipad(const struct wi_list *list, const struct wi_list_entry *e)
{
	const struct wi_list_style *style = list->style;

	return style->ipad ? style->ipad : DEFAULT_IPAD;
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


/* --- Shared clipping function -------------------------------------------- */


static void clip_bb(struct gfx_drawable *da, const struct wi_list_style *style,
    const struct gfx_rect *bb)
{
	struct gfx_rect clip = *bb;

	if (clip.y < (int) style->y0) {
		clip.h -= style->y0 - clip.y;
		clip.y = style->y0;
	}
	if (clip.y + clip.h > (int) style->y1 + 1)
		clip.h = style->y1 + 1 - clip.y;
	gfx_clip(da, &clip);
}


/* --- Render callback ----------------------------------------------------- */


void wi_list_render_entry(struct wi_list *list, struct wi_list_entry *entry)
{
	struct gfx_rect bb = {
		.x = 0,
		.y = list->style->y0 - list->up,
		.w = GFX_WIDTH,
		.h = entry_height(list, entry),
	};
	const struct wi_list_style *style = list->style;
	const struct wi_list_entry *e;
	bool odd = 0;

	if (!style->render)
		return;
	if (bb.h < (int) style->min_h)
		bb.h = style->min_h;
	for (e = list->list; e != entry; e = e->next) {
		unsigned h = entry_height(list, e);

		bb.y += style->min_h < h ? h : style->min_h;
		odd = !odd;
	}
	clip_bb(&main_da, style, &bb);
	list->style->render(list, entry, &main_da, &bb, odd);
	gfx_clip(&main_da, NULL);
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


/* --- List properties ----------------------------------------------------- */


bool list_is_empty(const struct wi_list *list)
{
	return !list->list;
}


/* --- List drawing -------------------------------------------------------- */


static void do_draw_entry(const struct wi_list *list,
    const struct wi_list_entry *e, struct gfx_drawable *da,
    const struct gfx_rect *bb, unsigned y, bool odd)
{
	const struct wi_list_style *style = list->style;

	clip_bb(da, style, bb);
	gfx_rect(da, bb, style->bg[odd]);
	text_text(da, 0, y + opad(list, e), e->first, list_font(list),
	    GFX_LEFT, GFX_TOP | GFX_MAX, style->fg[odd]);
	if (e->second)
		text_text(da, 0,
		    y + opad(list, e) + ipad(list, e) + list->text_height,
		    e->second, list_font(list),
		    GFX_LEFT, GFX_TOP | GFX_MAX, style->fg[odd]);
	if (style->render)
		style->render(list, e, da, bb, odd);
	gfx_clip(da, NULL);
}


static unsigned draw_entry(const struct wi_list *list,
    const struct wi_list_entry *e, struct gfx_drawable *da, int y, bool odd)
{
	const struct wi_list_style *style = list->style;
	unsigned h = entry_height(list, e);
	struct gfx_rect bb = { .x = 0, .y = y, .w = GFX_WIDTH, .h = h };
	int top = y; /* avoid going through "unsigned" */

	if (h < style->min_h) {
		y += (style->min_h - h) / 2;
		bb.h = style->min_h;
	}
	assert((unsigned) bb.h <= style->y1 - style->y0 + 1);

	if (top + (int) bb.h <= (int) style->y0)
		return bb.h;
	if (top > (int) style->y1)
		return bb.h;
	if (top >= (int) style->y0 && top + (int) bb.h - 1 <= (int) style->y1) {
		do_draw_entry(list, e, da, &bb, y, odd);
		return bb.h;
	}

	do_draw_entry(list, e, da, &bb, y, odd);
	return bb.h;
}


static unsigned draw_list(struct wi_list *list)
{
	const struct wi_list_style *style = list->style;
	const struct wi_list_entry *e;
	unsigned y = style->y0;
	unsigned i = 0;

//debug("  up %u\n", list->up);
	for (e = list->list; e; e = e->next) {
		unsigned h;

		h = draw_entry(list, e, &main_da, (int) y - (int) list->up,
		    i & 1);
		i++;
		y += h;
	}

	int ys = (int) y - (int) list->up;

	if (ys >= 0 && ys <= (int) style->y1)
		gfx_rect_xy(&main_da, 0, ys, GFX_WIDTH, style->y1 - ys + 1,
		    GFX_BLACK);
	return y - style->y0;
}


/* --- Vertical scrolling -------------------------------------------------- */


bool list_scroll(struct wi_list *list, int dy)
{
	const struct wi_list_style *style = list->style;

#if 0
debug("scrolling %u up %u scroll_from %u dy %d y0 %u y1 %u th %u\n",
    list->scrolling, list->up, list->scroll_from, dy, style->y0, style->y1,
    list->total_height);
#endif
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
	ui_update_display();
	return 1;
}


bool wi_list_moving(struct wi_list *list, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
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
	res = wi_list_moving(list, from_x, from_y, to_x, to_y, swipe);
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
			ui_update_display();
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
//	debug("height %d\n", q.h);

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
	unsigned y = list->style->y0 - list->up;
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
	draw_entry(list, entry, &main_da, y, odd);
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
