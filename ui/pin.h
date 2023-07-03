/*
 * pin.h - PIN definitions
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef PIN_H
#define	PIN_H

#define DUMMY_PIN		0xffff1234
#define MIN_PIN_LEN		4
#define MAX_PIN_LEN		8

#define	PIN_FREE_ATTEMPTS	3

#define	PIN_WAIT_MIN_S		60
#define	PIN_WAIT_MAX_S		3600
#define	PIN_WAIT_LOG2		6	/* log2(max / min) */

#define	PIN_WAIT_S(attempts) \
	((attempts) < PIN_FREE_ATTEMPTS ? 0 : \
	(attempts) > PIN_FREE_ATTEMPTS + PIN_WAIT_LOG2 ? PIN_WAIT_MAX_S : \
	PIN_WAIT_MIN_S << ((attempts) - PIN_FREE_ATTEMPTS))

#endif /* !PIN_H */
