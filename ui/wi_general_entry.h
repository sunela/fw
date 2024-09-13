/*
 * wi_general_entry.h - User interface: general text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef WI_GENERAL_ENTRY_H
#define WI_GENERAL_ENTRY_H

#include <stdbool.h>

#include "ui_entry.h"


struct wi_general_entry_ctx {
	struct ui_entry_input *input;
	const struct ui_entry_style *style;
	unsigned input_max_height;
	bool new;	/* this is a new device dialog */
};


extern struct ui_entry_ops wi_general_entry_ops;


/*
 * wi_general_entry_setup MUST be called by the caller of ui_entry, before
 * ui_entry calls wi_general_entry_init.
 */

void wi_general_entry_setup(struct wi_general_entry_ctx *ctx, bool new);

#endif /* !WI_GENERAL_ENTRY_H */
