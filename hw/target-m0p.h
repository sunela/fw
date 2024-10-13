/*
 * target-m0p.h - Board definitions: Sipeed M0P Dock
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ This is for an M0P Dock with the following additional devices:
 * - Display module (TFT controller on SPI, touch panel controller on I2C)
 * - BMA400 acceleration sensor on I2C
 * - ATECC608 Secure Element on I2C
 */

#ifndef TARGET_M0P_H
#define	TARGET_M0P_H

/* --- LED ----------------------------------------------------------------- */

#define LED		21	/* GPIO21_LED_PWM */

/* --- Buttons ------------------------------------------------------------- */

/*
 * Left and right if USB is "down" and the display is facing upward.
 *
 * M0P differentiates the buttons by analog voltage:
 *
 * S1     S2	 GPIO3 voltage
 * ------ ------ -------------
 * open   open	 1650 mV
 * closed open	  784 mV
 * -	  closed   32 mV
 *
 * The Sunela design has only one button, and we use GPIO3 as digital input.
 */

#define	BUTTON_R	3	/* GPIO3_ADCbutton */
#define	BUTTON_L	3	/* GPIO3_ADCbutton */

/* --- Display module: TFT ------------------------------------------------- */

#define LCD_CS		17	/* GPIO17_SPILCD_CS */
#define LCD_DnC		18	/* GPIO18_SPILCD_DC */
#define LCD_MOSI	19	/* GPIO19_SPILCD_MOSI */
#define LCD_SCLK	9	/* GPIO9_SPILCD_SCK */

#define LCD_RST		27	/* GPIO27_DVP_D3 */

#define LCD_SPI		0

#define LCD_WIDTH	240
#define LCD_HEIGHT	280

/* --- M0P I2C ------------------------------------------------------------- */

#define I2C0_SDA	1	/* GPIO1_TP&CAM_SDA */
#define I2C0_SCL	0	/* GPIO0_TP&CAM_SCL */

/* --- Display module: backlight ------------------------------------------- */

#define LCD_BL		16	/* GPIO16_DVP_PWDN */
#define	LCD_BL_INVERTED	1

/* --- Display module: touch screen ---------------------------------------- */

#define TOUCH_INT	34	/* GPIO34_DVP_D7 */

#define TOUCH_I2C	0
#define TOUCH_I2C_ADDR	0x15

/* --- BMA400 -------------------------------------------------------------- */

#define ACCEL_INT	33	/* GPIO33_DVP_D6 */

#define ACCEL_I2C	0
#define ACCEL_ADDR	0x14

/* --- ATECC608 ------------------------------------------------------------ */

#define ATECC_I2C	0
#define ATECC_ADDR	0x60

#endif /* !TARGET_M0P_H */
