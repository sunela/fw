/*
 * bl.c - Backlight driver
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ should be a PWM driver
 */

#include <stdbool.h>

#include "gpio.h"
#include "bl.h"


unsigned bl_pin;


void bl_on(bool on)
{
	gpio_out(bl_pin, on);
}


void bl_init(unsigned pin)
{
	bl_pin = pin;
	gpio_cfg_out(pin, 0, 0);
}
