/*
 * version.c - Build version and date
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#include <stdbool.h>
#include <stdint.h>

#include "version.h"


const char *build_date = BUILD_DATE;
uint32_t build_hash = BUILD_HASH;
bool build_dirty = BUILD_DIRTY;
