/*
 * version.h - Build version and date
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef VERSION_H
#define	VERSION_H

#include <stdbool.h>
#include <stdint.h>


extern const char *build_date;
extern uint32_t build_hash;
extern bool build_dirty;

#endif /* !VERSION_H */
