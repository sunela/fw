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
void backlight_init(unsigned pin);

#endif /* !BACKLIGHT_H */
