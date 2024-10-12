/*
 * backlight.c - Backlight driver
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ should be a PWM driver
 */

#include <stdbool.h>

#include "gpio.h"
#include "backlight.h"


static unsigned backlight_pin;
static bool backlight_inverted;


void backlight_on(bool on)
{
	gpio_out(backlight_pin, on ^ backlight_inverted);
}


void backlight_init(unsigned pin, bool inverted)
{
	backlight_pin = pin;
	backlight_inverted = inverted;
	gpio_cfg_out(pin, inverted, 0);
}
