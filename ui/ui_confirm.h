/*
 * ui_confirm.h - User interface: Confirm an action
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_CONFIRM_H
#define	UI_CONFIRM_H

#include <stdbool.h>

#include "db.h"


struct ui_confirm_params {
	const char	*action;
	const char	*name;
	void		(*fn)(void *user, bool confirm);
	void		*user;
};

#endif /* !UI_CONFIRM_H */
