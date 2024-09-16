/*
 * secrets.h - Master secret and related secrets
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SECRETS_H
#define	SECRETS_H

#include <stdbool.h>
#include <stdint.h>


#define	MASTER_SECRET_BYTES	32


extern uint8_t device_secret[MASTER_SECRET_BYTES];
extern uint8_t master_secret[MASTER_SECRET_BYTES];


bool secrets_change(uint32_t old_pin, uint32_t new_pin);
bool secrets_setup(uint32_t pin);
bool secrets_new(uint32_t pin);

void secrets_test_pad(void);

bool secrets_init(void);

#endif /* !SECRETS_H */
