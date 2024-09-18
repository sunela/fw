/*
 * pin.h - PIN operations
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef PIN_H
#define	PIN_H

#include <stdbool.h>
#include <stdint.h>

#include "secrets.h"


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


extern uint8_t pin_shuffle[10];


uint32_t pin_encode(const char *s);
void pin_success(void);
void pin_fail(void);
bool pin_revalidate(uint32_t pin);
unsigned pin_cooldown_ms(void);

/*
 * pin_change returns the following results:
 * -1 the change was attempted, but an IO error occurred
 * 0  the old PIN is incorrect or the new PIN is not acceptable
 * 1  the PIN was changed
 */

int pin_change(uint32_t old_pin, uint32_t new_pin);
bool pin_set(uint32_t new_pin);

void pin_shuffle_pad(void);

#endif /* !PIN_H */
