/*
 * st7789.c - Driver for the ST7789V TFT controller
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "hal.h"
#include "gpio.h"
#include "spi.h"
#include "st7789.h"
#include "debug.h"


#define	ST7789_ROWS	320

static unsigned st7789_spi;
static unsigned st7789_dnc;
static unsigned	st7789_xoff;
static unsigned	st7789_yoff;
static unsigned	st7789_width;
static unsigned	st7789_height;


/* --- ST7789V commands ---------------------------------------------------- */


#define	ST7789_SLPOUT	0x11	// Sleep Out
#define	ST7789_PTLON	0x12	// Partial Display Mode On
#define	ST7789_NORON	0x13	// Normal Display Mode On
#define	ST7789_INVON	0x21	// Display Inversion On
#define	ST7789_DISPON	0x29	// Display On
#define	ST7789_CASET	0x2a	// Column Address Set
#define	ST7789_RASET	0x2b	// Row Address Set
#define	ST7789_RAMWR	0x2c	// Memory Write
#define	ST7789_PTLAR	0x30	// Partial Area
#define	ST7789_VSCRDEF	0x33	// Vertical Scrolling Definition
#define	ST7789_MADCTL	0x36	// Memory Data Access Control
#define	ST7789_VSCSAD	0x37	// Vertical Scroll Start Address of RAM
#define	ST7789_COLMOD	0x3a	// Interface Pixel Format


/* --- Send commands and parameters ---------------------------------------- */


static void st7789_cmd(uint8_t cmd)
{
	spi_start();
	gpio_out(st7789_dnc, 0);
	spi_send(&cmd, 1);
	spi_end();
}


static void st7789_cmd8(uint8_t cmd, uint8_t value)
{
	spi_start();
	gpio_out(st7789_dnc, 0);
	spi_send(&cmd, 1);
	spi_sync();
	gpio_out(st7789_dnc, 1);
	spi_send(&value, 1);
	spi_end();
}


static void st7789_cmd16(uint8_t cmd, uint16_t value)
{
	uint8_t tmp[] = { value >> 8, value };

	spi_start();
	gpio_out(st7789_dnc, 0);
	spi_send(&cmd, 1);
	spi_sync();
	gpio_out(st7789_dnc, 1);
	spi_send(tmp, 2);
	spi_end();
}


static void st7789_cmd32(uint8_t cmd, uint32_t value)
{
	uint8_t tmp[] = { value >> 24, value >> 16, value >> 8, value };

	spi_start();
	gpio_out(st7789_dnc, 0);
	spi_send(&cmd, 1);
	spi_sync();
	gpio_out(st7789_dnc, 1);
	spi_send(tmp, 4);
	spi_end();
}


/* --- Send a block of data ------------------------------------------------ */


static void st7789_send_begin(uint8_t cmd)
{
	spi_start();
	gpio_out(st7789_dnc, 0);
	spi_send(&cmd, 1);
	spi_sync();
	gpio_out(st7789_dnc, 1);
}


static void st7789_send_data(const void *buf, unsigned len)
{
	spi_send(buf, len);
}


static void st7789_send_end(void)
{
	spi_end();
}


static void st7789_send(uint8_t cmd, const void *buf, unsigned len)
{
	st7789_send_begin(cmd);
	st7789_send_data(buf, len);
	st7789_send_end();
}


/* --- API ----------------------------------------------------------------- */


void st7789_update(const void *fb, unsigned x0, unsigned y0, unsigned x1,
    unsigned y1)
{
	st7789_cmd32(ST7789_CASET,
	    (x0 + st7789_xoff) << 16 | (x1 + st7789_xoff));
	st7789_cmd32(ST7789_RASET,
	    (y0 + st7789_yoff) << 16 | (y1 + st7789_yoff));

	if (x0 == 0 && x1 == st7789_width - 1) {
		st7789_send(ST7789_RAMWR, fb + y0 * st7789_width * 2,
		    st7789_width * (y1 - y0 + 1) * 2);
	} else {
		const void *p = fb + (y0 * st7789_width + x0) * 2;

		st7789_send_begin(ST7789_RAMWR);
		for (unsigned y = y0; y <= y1; y++) {
			st7789_send_data(p, (x1 - x0 + 1) * 2);
			p += st7789_width * 2;
		}
		st7789_send_end();
	}

	/* @@@ we need a small delay between frames */
	mdelay(1);
}


/*
 * Note that vertical scrolling only changes in which sequence lines are read
 * from memory when displaying a frame, but it does not change the memory
 * content itself.
 */

void st7789_vscroll_raw(uint16_t tfa, uint16_t vsa, uint16_t bfa, uint16_t vsp)
{
	uint8_t vscrdef[] = { tfa >> 8, tfa, vsa >> 8, vsa, bfa >> 8, bfa };

// debug("VR tfa %u vsa %u bfa %u vsp %u\n", tfa, vsa, bfa, vsp);

	assert(tfa + vsa + bfa == ST7789_ROWS);
	st7789_send(ST7789_VSCRDEF, vscrdef, sizeof(vscrdef));
	st7789_cmd16(ST7789_VSCSAD, vsp);
}


void st7789_vscroll(unsigned y0, unsigned y1, unsigned ytop)
{
	assert(y0 <= ytop);
	assert(ytop <= y1);
	assert(y1 < st7789_height);

// debug("VS y0 %u y1 %u ytop %u\n", y0, y1, ytop);
	/*
	 * Note: TFA is _below_ BFA, since the display is reversed.
	 *
	 * This also means that VSP is calculated from the bottom of the
	 * display.
	 */

	uint16_t tfa = ST7789_ROWS - (st7789_yoff + y1 + 1);
	uint16_t vsa = y1 - y0 + 1;
	uint16_t bfa = st7789_yoff + y0;
	uint16_t vsp = tfa + (vsa - (ytop - y0)) % vsa;

	st7789_vscroll_raw(tfa, vsa, bfa, vsp);
}


void st7789_on(void)
{
	st7789_cmd(ST7789_DISPON);
	mdelay(1);
}


void st7789_init(unsigned spi, unsigned rst, unsigned dnc,
    unsigned width, unsigned height, unsigned xoff, unsigned yoff)
{
	st7789_spi = spi;
	st7789_dnc = dnc;
	st7789_xoff = xoff;
	st7789_yoff = yoff;
	st7789_width = width;
	st7789_height = height;

	gpio_cfg_out(rst, 1, 0);
	gpio_cfg_out(dnc, 1, 0);

	/* --- reset and wakeup --- */

	gpio_out(rst, 0);
	mdelay(1);
	gpio_out(rst, 1);
	mdelay(120);

	st7789_cmd32(ST7789_PTLAR, yoff << 16 | (height + yoff - 1));
	st7789_cmd(ST7789_PTLON);		// enable partial mode

	st7789_cmd(ST7789_SLPOUT);		// exit sleep mode
	mdelay(120);				// SLPOUT-SLPIN timing

	/* --- display organization --- */

	st7789_cmd8(ST7789_COLMOD, 5 << 4 | 5);
		// 65k RGB interface; 16 bit/pixel control interface
	st7789_cmd8(ST7789_MADCTL, 1 << 7 | 1 << 6);
		// MY: page address mode bottom to top
		// MX: column address order: right to left
		// MV: page/column order: normal
		// ML: line address order: LCD refresh top to bottom
		// RGB: RGB (not BGR)
		// MH: display data latch data order: LCD refresh left to right
	st7789_cmd(ST7789_INVON);
//	st7789_cmd(ST7789_NORON);
}
