/*
 * assert.h - assert(3) replacement
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * assert(3) of the SDK crashes in fprintf before printing anything. This hack
 * is more useful.
 */

#ifdef SDK

#ifndef ASSERT_H
#define	ASSERT_H

#include <stdlib.h>

#include "debug.h"


#define	assert(c)							\
	do {								\
		if (!(c)) {						\
			debug("assertion failed %s:%u: \"%s\"\n",	\
			    __FILE__, __LINE__, #c);			\
			while (1);					\
		}							\
	} while (0)

#endif /* !ASSERT_H */

#else /* SDK */

#include_next <assert.h>

#endif /* !SDK */
