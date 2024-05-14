/*
 * rnd.c - Random number generators
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ On the final device, use more entropy sources.
 */

#include <stdint.h>

#include "rnd.h"


#ifdef SIM


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>


#define	DEV_RANDOM	"/dev/random"


void rnd_bytes(void *buf, unsigned size)
{
	static int fd = -1;

	if (fd == -1) {
		fd = open(DEV_RANDOM, O_RDONLY);
		if (fd < 0) {
			perror("open " DEV_RANDOM);
			exit(1);
		}
	}
	while (size) {
		ssize_t got;

		got = read(fd, buf, size);
		if (got < 0) {
			perror("read " DEV_RANDOM);
			exit(1);
		}
		if (!got) {
			fprintf(stderr, "got no data from " DEV_RANDOM);
			exit(1);
		}
		buf += got;
		size -= got;
	}
}


#else /* SIM */


#include "bl808/trng.h"


void rnd_bytes(void *buf, unsigned size)
{
	trng_read(buf, size);
}


#endif /* !SIM */


uint32_t rnd(uint32_t range)
{
	uint64_t tmp;

	rnd_bytes(&tmp, sizeof(tmp));
        return tmp % range;
}
