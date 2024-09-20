/*
 * ui_choices.h - User interface: List of page choices
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_CHOICES_H
#define	UI_CHOICES_H

#include <stdbool.h>


struct ui_choice {
	const char *label;
	const struct ui *ui;
	void *params;
};

struct ui_choices_params {
	const char *title;
	unsigned n_choices;
	const struct ui_choice *choices;
	bool call;
};

#endif /* !UI_CHOICES_H */
