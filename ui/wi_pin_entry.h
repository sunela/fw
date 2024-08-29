/*
 * wi_pin_entry.h - User interface: general text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef WI_PIN_ENTRY_H
#define WI_PIN_ENTRY_H

#include <stdbool.h>
#include <stdint.h>

#include "ui_entry.h"


struct wi_pin_entry_ctx {
	struct ui_entry_input *input;
	const struct ui_entry_style *style;
	bool login;	/* this is the login dialog */
	uint8_t shuffle[10];
};


extern struct ui_entry_ops wi_pin_entry_ops;


/*
 * wi_pin_entry_setup MUST be called by the caller of ui_entry, before
 * ui_entry calls wi_pin_entry_init. If "shuffle" is NULL, no shuffling takes
 * place.
 */

void wi_pin_entry_setup(struct wi_pin_entry_ctx *c, bool login,
    const uint8_t *shuffle);

#endif /* !WI_PIN_ENTRY_H */
