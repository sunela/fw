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


/*
 * Arbitrary but static build information for use in regression tests.
 */

#define	STATIC_BUILD_DATE	"20240823-01:08"
#define	STATIC_BUILD_HASH	0x12345678
#define	STATIC_BUILD_DIRTY	0

extern bool build_override;
extern const char *build_date;
extern uint32_t build_hash;
extern bool build_dirty;

#endif /* !VERSION_H */
