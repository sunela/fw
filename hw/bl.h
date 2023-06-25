/*
 * bl.h - Backlight driver
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef BL_H
#define BL_H

#include <stdbool.h>


void bl_on(bool on);
void bl_init(unsigned pin);

#endif /* !BL_H */
