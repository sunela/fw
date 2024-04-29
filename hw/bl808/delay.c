/*
 * delay.c - Delay loops
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "hal.h"


#define DELAY_LOOP	239	/* cycles per us */


void mdelay(unsigned ms)
{
#ifdef SDK
	msleep(ms);
#else /* SDK */
	unsigned i;

	while (ms--)
		for (i = 0; i != 1000 * DELAY_LOOP; i++)
			asm("");
#endif /* !SDK */
}
