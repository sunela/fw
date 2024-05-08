#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "usbd_core.h"
#include "bflb_mtimer.h"
#include "bflb_rtc.h"
#include "bflb_uart.h"
#include "bflb_flash.h"
#include "board.h"

#include "../sdk-hal.h"
#include "../db/storage.h"


/*
 * 16 MB Flash (M1s). We reserve the first half for the SDK and use the second
 * half for data storage. Of the storage area, we use - at least for now - two
 * partitions of 1 MB each.
 */

#define	FLASH_STORAGE_BASE	(8 * 1024 * 1024)
#define	FLASH_STORAGE_SIZE	(2 * 1024 * 1024)


void sunela_usb_init(void);


void mdelay(unsigned ms)
{
	bflb_mtimer_delay_ms(ms);
}


void msleep(unsigned ms)
{
	mdelay(ms);
}


uint64_t time_us(void)
{
	static struct bflb_device_s *rtc = NULL;

	if (!rtc) {
		rtc = bflb_device_get_by_name("rtc");
		/* enable RTC and set time */
		bflb_rtc_set_time(rtc, 0);
	}
	return bflb_rtc_get_time(rtc) / 32768.8 * 1000000;
}


/*
 * @@@ The SDK freezes during Flash operations. The cause may be that it runs
 * interrupt handlers, which in turn try to access XIP code. If the Flash is
 * busy with erase/write at that time, this can't end well ...
 *
 * @@@ A second problem is that writes can have byte granularity, with 256
 * bytes being the underlying physical size, but erasing is per sector (4 kB)
 * or block (even larger).
 *
 * We therefore need to either increase the storage block size to 4 kB, which
 * would probably be excessive, or organize the memory such that our access
 * pattern is compatible with Flash contraints.
 */

void console_char(char c)
{
	if (c == '\n')
		console_char('\r');
#if 0
	static struct bflb_device_s *console = NULL;

	if (!console)
		console = bflb_device_get_by_name("uart0");
	bflb_uart_put(console, (uint8_t *) &c, 1);
#endif
volatile uint32_t *base = (void *) 0x2000a000;	// ACM1 (M0)

while (!(base[0x21] & 0x3f));
base[0x22] = c;
}


bool storage_read_block(void *buf, unsigned n)
{
	uint32_t addr = FLASH_STORAGE_BASE + n * STORAGE_BLOCK_SIZE;
	int ret;

	assert(n < FLASH_STORAGE_SIZE / STORAGE_BLOCK_SIZE);
	ret = bflb_flash_read(addr, buf, STORAGE_BLOCK_SIZE);
debug("read (%d 0x%08lx) %d\n", n, (unsigned long) addr, ret);
	return !ret;
}


/* @@@ the W25Q128JVEJQ Flash in M1s has 4 kB sectors, so each "block" erase
 * erases four blocks. */

bool storage_write_block(const void *buf, unsigned n)
{
	uint32_t addr = FLASH_STORAGE_BASE + n * STORAGE_BLOCK_SIZE;

/* @@@ we probably need to disable interrupts while erasing / writing Flash */
	assert(n < FLASH_STORAGE_SIZE / STORAGE_BLOCK_SIZE);
	if (bflb_flash_erase(addr, STORAGE_BLOCK_SIZE))
		return 0;
	return !bflb_flash_write(addr, (void *) buf, STORAGE_BLOCK_SIZE);
}


unsigned storage_blocks(void)
{
	return FLASH_STORAGE_SIZE / STORAGE_BLOCK_SIZE;
}


int main(void)
{
	board_init();
	sunela_usb_init();
	sdk_main();
}
