/*
 * uw_list.h - User interface widget: scrollable list of tappable items
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UW_LIST_H
#define	UW_LIST_H

#include "gfx.h"
#include "text.h"


struct uw_list_entry;
struct uw_list;

struct uw_list_style {
	unsigned		y0, y1;	/* display area for the list */
	gfx_color		fg[2];	/* even and odd entries */
	gfx_color		bg[2];	/* even and odd entries */
	unsigned		opad;	/* padding at top and bottom (0 -> 1) */
	unsigned		ipad;	/* padding between lines (0 -> 1)*/
	const struct font *font;/* NULL for default */
	void (*render)(const struct uw_list *list,
	    const struct uw_list_entry *entry, const struct gfx_rect *bb,
	    bool odd);
};

struct uw_list {
	struct uw_list_entry		*list;
	struct uw_list_entry		**anchor;
	const struct uw_list_style	*style;
	unsigned			text_height;
};


struct uw_list_entry *uw_list_pick(const struct uw_list *list,
    unsigned x, unsigned y);
void *uw_list_user(const struct uw_list_entry *entry);

/*
 * uw_list_render only calls the "render" callback (if set), but performs no
 * other operations.
 */
void uw_list_render(struct uw_list *list, struct uw_list_entry *entry);

void uw_list_forall(struct uw_list *list,
    void (*fn)(struct uw_list *list, struct uw_list_entry *entry, void *user),
    void *user);

void uw_list_begin(struct uw_list *ctx, const struct uw_list_style *style);
struct uw_list_entry *uw_list_add(struct uw_list *ctx,
    const char *first, const char *second, void *user);
void uw_list_update_entry(struct uw_list *list, struct uw_list_entry *entry,
    const char *first, const char *second, void *user);
void uw_list_end(struct uw_list *ctx);

void uw_list_destroy(struct uw_list *ctx);

#endif /* !UW_LIST_H */
