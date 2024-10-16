/*
 * target-m1s.h - Board definitions: Sipeed M1s Dock
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ This is for an M1s Dock with the following additional devices:
 * - Display module (TFT controller on SPI, touch panel controller on I2C)
 * - BMA400 acceleration sensor on I2C
 * - ATECC608 Secure Element on I2C
 */

#ifndef TARGET_M1S_H
#define	TARGET_M1S_H

/* --- LED ----------------------------------------------------------------- */

#define LED		8	/* IO8_PWM_LED */

/* --- Buttons ------------------------------------------------------------- */

/* Left and right if USB is "down" and the display is facing upward */

#define	BUTTON_R	22	/* IO22_Button1 */
#define	BUTTON_L	23	/* IO23_Button2 */

/* --- Display module: TFT ------------------------------------------------- */

#define LCD_CS		12	/* IO12_LCD_DBI_CS */
#define LCD_DnC		13	/* IO13_LCD_DBI_DC */
#define LCD_MOSI	25	/* IO25_LCD_DBI_SDA */
#define LCD_SCLK	19	/* IO19_LCD_DBI_SCK */

#define LCD_RST		24	/* IO24_LCD_RESET */

#define LCD_SPI		0

#define LCD_WIDTH	240
#define LCD_HEIGHT	280

/* --- M1s I2C ------------------------------------------------------------- */

#define I2C0_SDA	7	/* IO7_CAM_I2C0_SDA */
#define I2C0_SCL	6	/* IO7_CAM_I2C0_SDA */

/* --- Display module: backlight ------------------------------------------- */

#define LCD_BL		11	/* IO11_LCD_BL_PWM */
#define	LCD_BL_INVERTED	0

/* --- Display module: touch screen ---------------------------------------- */

#define TOUCH_INT	32	/* IO32_TP_TINT */

#define TOUCH_I2C	0
#define TOUCH_I2C_ADDR	0x15

/* --- BMA400 -------------------------------------------------------------- */

#define ACCEL_INT	31	/* IO31 */

#define ACCEL_I2C	0
#define ACCEL_ADDR	0x14

/* --- ATECC608 ------------------------------------------------------------ */

#define ATECC_I2C	0
#define ATECC_ADDR	0x60

#endif /* !TARGET_M1S_H */
