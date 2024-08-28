/*
 * wi_pin_entry.h - User interface: general text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef WI_PIN_ENTRY_H
#define WI_PIN_ENTRY_H

#include "ui_entry.h"


struct wi_pin_entry_ctx {
	struct ui_entry_input *input;
	const struct ui_entry_style *style;
};


extern struct ui_entry_ops wi_pin_entry_ops;

#endif /* !WI_PIN_ENTRY_H */
