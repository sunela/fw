/*
 * alloc.h - Common allocation functions
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef LIB_ALLOC_H
#define LIB_ALLOC_H

#include <stdlib.h>
#include <stdio.h>


#define alloc_size(s)					\
    ({  void *alloc_size_tmp = malloc(s);		\
	if (!alloc_size_tmp) {				\
		perror("malloc");			\
		exit(1);				\
	}						\
	alloc_size_tmp; })


#define alloc_type(t) ((t *) alloc_size(sizeof(t)))
#define alloc_type_n(t, n) ((t *) alloc_size(sizeof(t) * (n)))

#endif /* !LIB_ALLOC_H */
