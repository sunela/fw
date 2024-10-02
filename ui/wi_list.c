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

/*
 * OVER_SCROLL determines how far we scroll past the end of the list. The main
 * purpose is to let us move the bottom item out of the area where the screen
 * corners may cut off part of the content.
 *
 * Another use is to have an "at the end" position from which to invoke an
 * overlay, e.g., when moving entries.
 */
#define	OVER_SCROLL	50


struct wi_list_entry {
	const char *first;
	const char *second;
	unsigned left;	/* horizontal scrolling */
	unsigned first_w, second_w;
	void *user;
	const struct wi_list_entry_style *style;
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
	struct wi_list_entry *e;
	int pos = list->y0 - list->up;

	if ((int) y < pos)
		return NULL;
	for (e = list->list; e; e = e->next) {
		const struct wi_list_entry_style *entry_style = e->style;
		unsigned h = entry_height(list, e);

		pos += entry_style->min_h < h ? h : entry_style->min_h;
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


static void clip_bb(struct gfx_drawable *da, const struct wi_list *list,
    const struct gfx_rect *bb)
{
	struct gfx_rect clip = *bb;

	if (clip.y < (int) list->y0) {
		clip.h -= list->y0 - clip.y;
		clip.y = list->y0;
	}
	if (clip.y + clip.h > (int) list->style->y1 + 1)
		clip.h = list->style->y1 + 1 - clip.y;
	gfx_clip(da, &clip);
}


/* --- Render callback ----------------------------------------------------- */


void wi_list_render_entry(struct wi_list *list, struct wi_list_entry *entry)
{
	struct gfx_rect bb = {
		.x = 0,
		.y = list->y0 - list->up,
		.w = GFX_WIDTH,
		.h = entry_height(list, entry),
	};
	const struct wi_list_entry_style *entry_style = entry->style;
	const struct wi_list_entry *e;
	bool odd = 0;

	if (!entry_style->render)
		return;
	if (bb.h < (int) entry_style->min_h)
		bb.h = entry_style->min_h;
	for (e = list->list; e != entry; e = e->next) {
		const struct wi_list_entry_style *es = e->style;
		unsigned h = entry_height(list, e);

		bb.y += es->min_h < h ? h : es->min_h;
		odd = !odd;
	}
	clip_bb(&main_da, list, &bb);
	entry_style->render(list, entry, &main_da, &bb, odd);
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
	const struct wi_list_entry_style *entry_style = e->style;

	clip_bb(da, list, bb);
	gfx_rect(da, bb, entry_style->bg[odd]);
//debug("w %u %u off %u\n", e->first_w, e->second_w, e->left);
	text_text(da, e->first_w <= GFX_WIDTH ? 0 : -e->left,
	    y + opad(list, e),
	    e->first, list_font(list), GFX_LEFT, GFX_TOP | GFX_MAX,
	    entry_style->fg[odd]);
	if (e->second)
		text_text(da, e->second_w <= GFX_WIDTH ? 0 : -e->left,
		    y + opad(list, e) + ipad(list, e) + list->text_height,
		    e->second, list_font(list),
		    GFX_LEFT, GFX_TOP | GFX_MAX, entry_style->fg[odd]);
	if (entry_style->render)
		entry_style->render(list, e, da, bb, odd);
	gfx_clip(da, NULL);
}


static unsigned draw_entry(const struct wi_list *list,
    const struct wi_list_entry *e, struct gfx_drawable *da, int y, bool odd)
{
	const struct wi_list_style *style = list->style;
	const struct wi_list_entry_style *entry_style = e->style;
	unsigned h = entry_height(list, e);
	struct gfx_rect bb = { .x = 0, .y = y, .w = GFX_WIDTH, .h = h };
	int top = y; /* avoid going through "unsigned" */

	if (h < entry_style->min_h) {
		y += (entry_style->min_h - h) / 2;
		bb.h = entry_style->min_h;
	}
	assert((unsigned) bb.h <= style->y1 - list->y0 + 1);

	if (top + (int) bb.h <= (int) list->y0)
		return bb.h;
	if (top > (int) style->y1)
		return bb.h;
	if (top >= (int) list->y0 && top + (int) bb.h - 1 <= (int) style->y1) {
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
	unsigned y = list->y0;
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
	return y - list->y0;
}


/* --- Vertical scrolling -------------------------------------------------- */


/*
 * Negative dx scrolls left, negative dy scrolls up.
 */

static bool list_scroll(struct wi_list *list, int dx, int dy)
{
	const struct wi_list_style *style = list->style;

#if 0
debug("scrolling %u up %u scroll_up %u dy %d y0 %u y1 %u th %u\n",
    list->scrolling, list->up, list->scroll_up, dy, list->y0, style->y1,
    list->total_height);
#endif
	int up = list->scroll_up - dy;

	if (up < 0) {
		up = 0;
	} else {
		unsigned list_h = list->total_height + OVER_SCROLL;
		unsigned win_h = style->y1 - list->y0 + 1;

		if (list_h < win_h)
			up = 0;
		else if (list_h - win_h < (unsigned) up)
			up = list_h - win_h;
	}
	list->up = up;

	struct wi_list_entry *e = list->scroll_entry;

	if (e) {
		unsigned width =
		    e->first_w > e->second_w ? e->first_w : e->second_w;
		unsigned max_left;
		int left = list->scroll_left - dx;

		if (width <= GFX_WIDTH)
			max_left = 0;
		else
			max_left = width - GFX_WIDTH;

		if (left < 0)
			left = 0;
		if (left > (int) max_left)
			left = max_left;
		e->left = left;
	}
	draw_list(list);
	ui_update_display();
	return 1;
}


bool wi_list_moving(struct wi_list *list, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	const struct wi_list_style *style = list->style;

	if (from_y < list->y0 || from_y > style->y1)
		return 0;
	if (!list->scrolling) {
		struct wi_list_entry *e = wi_list_pick(list, from_x, from_y);

		/*
		 * Only consider horizontal scrolling if anything needs
		 * scrolling. This way, things like "left-swipe to go back"
		 * work most of the time. (Horizontal scrolling takes
		 * precedence over left-swipe.)
		 */
		if (e && (e->first_w > GFX_WIDTH || e->second_w > GFX_WIDTH)) {
			list->scroll_entry = e;
			if (e)
				list->scroll_left = e->left;
		} else {
			list->scroll_entry = NULL;
		}
		list->scroll_up = list->up;
		list->scrolling = 1;
	}
	if (from_x != to_x || from_y != to_y)
		list_scroll(list, to_x - from_x, to_y - from_y);
	return 1;
}


bool wi_list_to(struct wi_list *list, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	bool res;

	if (!list->scroll_entry && swipe == us_left) {
		wi_list_cancel(list);
		return 0;
	}
	res = wi_list_moving(list, from_x, from_y, to_x, to_y, swipe);
	list->scrolling = 0;
	list->scroll_entry = NULL;
	return res;
}


void wi_list_cancel(struct wi_list *list)
{
debug("wi_list_cancel\n");
	if (list->scrolling) {
		if (list->scroll_entry)
			list->scroll_entry->left = 0;
		if (list->scroll_up != list->up || list->scroll_entry) {
			list->scroll_entry = NULL;
			list->up = list->scroll_up;
			draw_list(list);
			ui_update_display();
		}
		list->scrolling = 0;
	}
}


/* --- Relocate list (e.g., for vertical centering) ------------------------ */


void wi_list_y0(struct wi_list *list, unsigned y0)
{
	list->y0 = y0;
	draw_list(list);
}


/* --- List construction, update, destruction ------------------------------ */


void wi_list_begin(struct wi_list *list, const struct wi_list_style *style)
{
	struct text_query q;

	list->style = style;
	list->y0 = style->y0;

	text_query(0, 0, "", list_font(list),
	    GFX_TOP | GFX_MAX, GFX_TOP | GFX_MAX, &q);
	list->text_height = q.h;
//	debug("height %d\n", q.h);

	list->list = NULL;
	list->anchor = &list->list;
	list->total_height = 0;
	list->scrolling = 0;
	list->scroll_entry = NULL;
}


static unsigned get_w(const struct wi_list *list, const char *s)
{
	struct text_query q;

	if (!s)
		return 0;
	text_query(0, 0, s, list_font(list), GFX_LEFT, GFX_TOP, &q);
	return q.w;
}


struct wi_list_entry *wi_list_add(struct wi_list *list,
    const char *first, const char *second, void *user)
{
	struct wi_list_entry *e;

	assert(first);
	e = alloc_type(struct wi_list_entry);
	e->first = first ? stralloc(first) : first;
	e->second = second ? stralloc(second) : second;
	e->left = 0;
	e->first_w = get_w(list, first);
	e->second_w = get_w(list, second);
	e->user = user;
	e->style = &list->style->entry;
	e->next = NULL;
	*list->anchor = e;
	list->anchor = &e->next;
	return e;
}


void wi_list_update_entry(struct wi_list *list, struct wi_list_entry *entry,
    const char *first, const char *second, void *user)
{
	bool changed = (first ? !entry->first || strcmp(first, entry->first) :
	    !!entry->first) ||
	    (second ? !entry->second || strcmp(second, entry->second) :
	    !!entry->second);
	const struct wi_list_entry *e;
	unsigned y = list->y0 - list->up;
	int odd = 0;

	entry->user = user;
	if (!changed)
		return;

	free((void *) entry->first);
	free((void *) entry->second);
	entry->first = first ? stralloc(first) : first;
	entry->left = 0;
	entry->second = second ? stralloc(second) : second;
	entry->first_w = get_w(list, first);
	entry->second_w = get_w(list, second);

	for (e = list->list; e != entry; e = e->next) {
		const struct wi_list_entry_style *es = e->style;
		unsigned h = entry_height(list, e);

		y += h < es->min_h ? es->min_h : h;
		odd = !odd;
	}
	draw_entry(list, entry, &main_da, y, odd);
}


void wi_list_entry_style(struct wi_list *list, struct wi_list_entry *entry,
    const struct wi_list_entry_style *style)
{
	entry->style = style ? style : &list->style->entry;
}


unsigned wi_list_end(struct wi_list *list)
{
	list->total_height = draw_list(list);
	return list->total_height;
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
