/*
 * wi_general_entry.h - User interface: general text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef WI_GENERAL_ENTRY_H
#define WI_GENERAL_ENTRY_H

#include "ui_entry.h"


struct wi_general_entry_ctx {
	struct ui_entry_input *input;
	const struct ui_entry_style *style;
	unsigned input_max_height;
};


extern struct ui_entry_ops wi_general_entry_ops;

#endif /* !WI_GENERAL_ENTRY_H */
