/*
 * wi_list.h - Widget: scrollable list of tappable items
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef WI_LIST_H
#define	WI_LIST_H

#include <stdbool.h>

#include "gfx.h"
#include "text.h"
#include "ui.h"


struct wi_list_entry;
struct wi_list;

struct wi_list_entry_style {
	gfx_color		fg[2];	/* even and odd entries */
	gfx_color		bg[2];	/* even and odd entries */
	unsigned		min_h;	/* including padding */
	void (*render)(const struct wi_list *list,
	    const struct wi_list_entry *entry, struct gfx_drawable *da,
	    const struct gfx_rect *bb, bool odd);
};

struct wi_list_style {
	unsigned		y0, y1;	/* display area for the list */
	unsigned		opad;	/* padding at top and bottom (0 -> 1) */
	unsigned		ipad;	/* padding between lines (0 -> 1)*/
	unsigned		no_over_scroll; /* suppress over-scroll */
	const struct font *font;/* NULL for default */
	struct wi_list_entry_style entry;
};

struct wi_list {
	struct wi_list_entry	*list;
	struct wi_list_entry	**anchor;
	const struct wi_list_style *style;
	unsigned		y0; /* defaults to style->y0 */
	unsigned		text_height;
	unsigned		total_height;	/* set by wi_list_end */
	unsigned		up;		/* distance scrolled up */
	bool			scrolling;	/* vertical scrolling */
	unsigned		scroll_up;
	struct wi_list_entry	*scroll_entry;	/* horizontal scrolling */
	unsigned		scroll_left;
};


struct wi_list_entry *wi_list_pick(const struct wi_list *list,
    unsigned x, unsigned y);
void *wi_list_user(const struct wi_list_entry *entry);

/*
 * wi_list_render_entry only calls the "render" callback (if set), but performs
 * no other operations.
 */
void wi_list_render_entry(struct wi_list *list, struct wi_list_entry *entry);

void wi_list_forall(struct wi_list *list,
    void (*fn)(struct wi_list *list, struct wi_list_entry *entry, void *user),
    void *user);

bool list_is_empty(const struct wi_list *list);

bool wi_list_moving(struct wi_list *list, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe);
bool wi_list_to(struct wi_list *list, unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe);
void wi_list_cancel(struct wi_list *list);

void wi_list_y0(struct wi_list *list, unsigned y0);

void wi_list_begin(struct wi_list *ctx, const struct wi_list_style *style);
struct wi_list_entry *wi_list_add_width(struct wi_list *ctx,
    const char *first, const char *second, unsigned width, void *user);
struct wi_list_entry *wi_list_add(struct wi_list *ctx,
    const char *first, const char *second, void *user);
void wi_list_update_entry(struct wi_list *list, struct wi_list_entry *entry,
    const char *first, const char *second, void *user);
void wi_list_entry_style(struct wi_list *list, struct wi_list_entry *entry,
    const struct wi_list_entry_style *style);
unsigned wi_list_end(struct wi_list *ctx);

void wi_list_destroy(struct wi_list *ctx);

#endif /* !WI_LIST_H */
