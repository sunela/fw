/*
 * delay.h - Delay loops
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef DELAY_H
#define	DELAY_H

#define DELAY_LOOP	239	/* cycles per us */


static inline void mdelay(unsigned ms)
{
	unsigned i;

	while (ms--)
		for (i = 0; i != 1000 * DELAY_LOOP; i++)
			asm("");
}

#endif /* !DELAY_H */
