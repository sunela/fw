/*
 * ui_rmt.h - User interface: Remote control
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_RMT_H
#define	UI_RMT_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "db.h"


struct ui_rmt_field {
	const struct db_entry *de;
	const struct db_field *f;
	bool binary;
};


bool ui_rmt_reveal(const struct ui_rmt_field *field);
bool ui_rmt_set_time(time_t new_time);

#endif /* !UI_RMT_H */
