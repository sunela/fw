/*
 * ui_setup.c - User interface: Setup
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "util.h"
#include "ui_choices.h"
#include "ui.h"


/* --- Open/close ---------------------------------------------------------- */


static void ui_setup_open(void *ctx, void *params)
{
	static const struct ui_choice choices_l2[] = {
		{ "Public key",		&ui_show_pubkey,	NULL },
		{ "Show secret",	&ui_show_master,	NULL },
		{ "Set secret",		&ui_set_master,		NULL },
	};
	static const struct ui_choices_params choice_params_l2 = {
		.title		= "Master Secret",
		.n_choices	= ARRAY_ENTRIES(choices_l2),
		.choices	= choices_l2,
		.call		= 1,
	};
	static const struct ui_choice choices[] = {
		{ "Change PIN",		&ui_pin_change,	NULL },
		{ "Time & date",	&ui_time,	NULL },
		{ "Master secret",	&ui_choices,
		    (void *) &choice_params_l2 },
		{ "Storage",		&ui_storage,	NULL },
		{ "Version", 		&ui_version,	NULL },
		{ "R&D",		&ui_rd,		NULL }
	};
	static const struct ui_choices_params choice_params = {
		.title		= "Setup",
		.n_choices	= ARRAY_ENTRIES(choices),
		.choices	= choices,
		.call		= 1,
	};
	set_idle(IDLE_SETUP_S);
	ui_switch(&ui_choices, (void *) &choice_params);
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_setup = {
	.name		= "setup",
	.open		= ui_setup_open,
};
