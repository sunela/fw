/*
 * backlight.h - Backlight driver
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <stdbool.h>


void backlight_on(bool on);

/*
 * If inverted, the backlight is on when the pin is "1". Otherwise, it is lit
 * when the pin is "0".
 */
void backlight_init(unsigned pin, bool inverted);

#endif /* !BACKLIGHT_H */
