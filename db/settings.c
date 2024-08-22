/*
 * settings.c - Settings record
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#include "settings.h"


struct settings settings = {
	.crosshair	= 0,
	.strict_rmt	= 0,
};


void settings_update(void)
{
}


void settings_load(void)
{
}
