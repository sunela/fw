/*
 * script.h - Functions for scripted tests and other debugging
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#ifndef SCRIPT_H
#define	SCRIPT_H

#include <stdbool.h>

#include "gfx.h"
#include "db.h"


void dump_db(const struct db *db, bool pointers);
bool screenshot(const struct gfx_drawable *da, const char *fmt, ...);
bool run_script(char **args, int n_args);

#endif /* !SCRIPT_H */
