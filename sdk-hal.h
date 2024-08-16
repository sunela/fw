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


/*
 * The bouffalo_sdk says, in
 * drivers/lhal/include/arch/risc-v/t-head/Core/Include/csi_rv64_gcc.h
 * drivers/lhal/include/arch/risc-v/t-head/Core/Include/core_rv32.h
 *
 * fence
 * icache.iall
 * fence
 *
 * But icache.iall isn't recognized by the toolchain, and ".long 0x0100000b"
 * for th.icache.iall produces an illegal instruction.
 *
 * Luckily, local_flush_icache_all from
 * lore.barebox.org/barebox/20221005111214.148844-5-m.felsch@pengutronix.de/
 * points in the rigfht direction: a simple flush.i will do, no special T-Head
 * magic needed, it seems.
 */

static inline void flush_cache(void)
{
	asm volatile ("fence.i" ::: "memory");
}


bool usb_query(uint8_t req, uint8_t **data, uint32_t *len);
bool usb_arrival(uint8_t req, const void *data, uint32_t len);

void sdk_main(void);

#endif /* !SDK_HAL_H */
