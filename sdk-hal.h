/*
 * sdk-hal.h - Hardware abstraction layer for the SDK
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SDK_HAL_H
#define	SDK_HAL_H

#include <stdbool.h>
#include <stdint.h>


bool usb_query(uint8_t req, uint8_t **data, uint32_t *len);
bool usb_arrival(uint8_t req, const void *data, uint32_t len);

void sdk_main(void);

#endif /* !SDK_HAL_H */
