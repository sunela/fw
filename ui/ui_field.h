/*
 * ui_field.h - User interface: Field operations
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_FIELD_H
#define	UI_FIELD_H

#include <stdbool.h>

#include "db.h"


struct ui_field_edit_params {
	struct db_entry *de;
	enum field_type type;
};


bool ui_field_more(const struct db_entry *de);

#endif /* !UI_FIELD_H */
