/*
 * st7789.h - Driver for the ST7789V TFT controller
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef ST7789_H
#define	ST7789_H

#include <stdint.h>


/*
 * R5 G6 B5, with byte swapping.
 *
 * RGB888:			 RRRRRxxx GGGgggxx BBBBBxxx
 *					     |
 * RGB88 and byte-swapped:		     v
 * RRRR.RGGG.gggB.BBBBB becomes gggB.BBBBB.RRRR.RGGG
 */

static inline uint16_t st7789_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	return (r & 0xf8) | (g & 0xe0) >> 5 | (g & 0x1c) << 12 |
	    (b & 0xf8) << 5;
}


void st7789_update(const void *fb, unsigned x0, unsigned y0, unsigned x1,
    unsigned y1);

void st7789_vscroll_raw(uint16_t tfa, uint16_t vsa, uint16_t bfa, uint16_t vsp);

/*
 * st7789_vscroll scrolls (rotates) displaying of the rows y0 to y1.
 * The row originallly at ytop is displayed at y0.
 */

void st7789_vscroll(unsigned y0, unsigned y1, unsigned ytop);

void st7789_on(void);
void st7789_init(unsigned spi, unsigned rst, unsigned dnc,
    unsigned width, unsigned height, unsigned xoff, unsigned yoff);

#endif /* !ST7789_H */
