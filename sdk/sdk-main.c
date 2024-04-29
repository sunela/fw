#include <stddef.h>
#include <stdint.h>

#include "usbd_core.h"
#include "bflb_mtimer.h"
#include "bflb_rtc.h"
#include "bflb_uart.h"
#include "board.h"
#include "../sdk-hal.h"


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


void console_char(char c)
{
	static struct bflb_device_s *console = NULL;

	if (!console)
		console = bflb_device_get_by_name("uart0");
	if (c == '\n')
		console_char('\r');
	bflb_uart_put(console, (uint8_t *) &c, 1);
}


int main(void)
{
	board_init();
	sunela_usb_init();
	sdk_main();
}
