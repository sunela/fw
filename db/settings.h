/*
 * settings.h - Settings record
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SETTINGS_H
#define	SETTINGS_H

#include <stdbool.h>
#include <stdint.h>


struct settings {
	bool	crosshair;	/* show a crosshair at the tap position */
	bool	strict_rmt;	/* strict RMT protocol (panic on error) */
};


extern struct settings settings;


bool settings_update(void);
bool settings_process(uint16_t seq, const void *payload, unsigned size);
void settings_reset(void);

#endif /* !SETTINGS_H */
