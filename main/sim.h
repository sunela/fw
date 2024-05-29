/*
 * sim.h - Hardware abstraction layer for running in a simulator on Linux
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SIM_H
#define	SIM_H

#include <stdbool.h>


extern bool headless;

extern const char *screenshot_name;
extern unsigned screenshot_number;

#endif /* !SIM_H */
