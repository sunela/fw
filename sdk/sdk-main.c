#include "usbd_core.h"
#include "bflb_mtimer.h"
#include "board.h"


extern void sunela_usb_init(void);


int main(void)
{
    board_init();

    sunela_usb_init();
    while (1) {
//        hid_keyboard_test();
        bflb_mtimer_delay_ms(500);
    }
}
