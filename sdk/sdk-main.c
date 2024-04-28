#include "usbd_core.h"
#include "bflb_mtimer.h"
#include "board.h"
#include "../sdk-hal.h"


void sunela_usb_init(void);


void msleep(unsigned ms)
{
	bflb_mtimer_delay_ms(1000 * ms);
}


int main(void)
{
	board_init();
	sunela_usb_init();
	sdk_main();
}
