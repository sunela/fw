/*
 * settings.h - Settings record
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SETTINGS_H
#define	SETTINGS_H

#include <stdbool.h>


struct settings {
	bool	crosshair;	/* show a crosshair at the tap position */
	bool	strict_rmt;	/* strict RMT protocol (panic on error) */
};


extern struct settings settings;


void settings_update(void);
void settings_load(void);

#endif /* !SETTINGS_H */
