/*
 * i2c.c - Driver for BL618/BL808 I2C
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * Proof of concept. Known issues:
 * - mystery delay needed after writes
 * - when writing, we preload all the data into the FIFO, limiting the
 *   maximum message size. We should just enable the master on FIFO full or at
 *   the end of the data loop, whichever comes first.
 * - the use of the subaddress functionality for the register value makes this
 *   driver unnecessarily complicated. We should try to turn off subaddresses
 *   and let higher layers worry about selecting registers. Unfortunately, it
 *   seems that the I2C IP wouldn't like such a setup, at least it doesn't with
 *   the current zero-data hack. More research is needed.
 * - we don't detect error conditions (NAK, timeout)
 */

#include <stdbool.h>
#include <stdint.h>
#include <strings.h>
#include <assert.h>

#include "hal.h"
#include "mmio.h"
#include "gpio.h"
#include "i2c.h"


/* BL618 has only I2C0 and I2C1. */

#define	I2C_BASE(i2c) \
	((i2c) == 0 ? mmio_m0_base + 0xa300 : \
	(i2c) == 1 ? mmio_m0_base + 0xa900 : \
	(i2c) == 2 ? mmio_d0_base + 0x3000 : mmio_d0_base + 0x4000)

#define	I2C_CONFIG(i2c) \
	(*(volatile uint32_t *) I2C_BASE(i2c))
#define	I2C_MASK_CFG_DEG_CNT		(0xf << 28)
#define	I2C_MASK_CFG_PKT_LEN		(0xff << 20)
#define	I2C_MASK_CFG_SLV_ADDR		(0x3ff << 8)
#define	I2C_MASK_CFG_10B_EN		(1 << 7)
#define	I2C_MASK_CFG_SUB_ADDR_LEN	(3 << 5)
enum I2C_SUB_ADDR_LEN {
	I2C_SUB_ADDR_LEN_1		= 0,
	I2C_SUB_ADDR_LEN_2		= 1,
	I2C_SUB_ADDR_LEN_3		= 2,
	I2C_SUB_ADDR_LEN_4		= 3,
};
#define	I2C_MASK_CFG_SUB_ADDR_EN	(1 << 4)
#define	I2C_MASK_CFG_SCL_SYNC_EN	(1 << 3)
#define	I2C_MASK_CFG_DEG_EN		(1 << 2)
#define	I2C_MASK_CFG_RnW		(1 << 1)
#define	I2C_MASK_CFG_M_EN		(1 << 0)

#define	I2C_INT_STS(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x4))
#define	I2C_MASK_INT_FER_EN		(1 << 29)	/* enable */
#define	I2C_MASK_INT_ARB_EN		(1 << 28)
#define	I2C_MASK_INT_NAK_EN		(1 << 27)
#define	I2C_MASK_INT_RXF_EN		(1 << 26)
#define	I2C_MASK_INT_TXF_EN		(1 << 25)
#define	I2C_MASK_INT_END_EN		(1 << 24)
#define	I2C_MASK_INT_ARB_CLR		(1 << 20)	/* clear */
#define	I2C_MASK_INT_NAK_CLR		(1 << 19)
#define	I2C_MASK_INT_RXF_CLR		(1 << 18)
#define	I2C_MASK_INT_TXF_CLR		(1 << 17)
#define	I2C_MASK_INT_END_CLR		(1 << 16)
#define	I2C_MASK_INT_FER_MASK		(1 << 13)	/* mask */
#define	I2C_MASK_INT_ARB_MASK		(1 << 12)
#define	I2C_MASK_INT_NAK_MASK		(1 << 11)
#define	I2C_MASK_INT_RXF_MASK		(1 << 10)
#define	I2C_MASK_INT_TXF_MASK		(1 << 9)
#define	I2C_MASK_INT_END_MASK		(1 << 8)
#define	I2C_MASK_INT_FER		(1 << 5)	/* status; FIFO error */
#define	I2C_MASK_INT_ARB		(1 << 4)	/* arbitration lost */
#define	I2C_MASK_INT_NAK		(1 << 3)	/* NACK */
#define	I2C_MASK_INT_RXF		(1 << 2)	/* RX FIFO ready */
#define	I2C_MASK_INT_TXF		(1 << 1)	/* TX FIFO ready */
#define	I2C_MASK_INT_END		(1 << 0)	/* transfer end */

#define	I2C_SUB_ADDR(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x8))

#define	I2C_BUS_BUSY(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0xc))
#define I2C_MASK_BUS_BUSY_CLR		(1 << 1)
#define I2C_MASK_BUS_BUSY		(1 << 0)

#define	I2C_PRD_START(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x10))
#define	I2C_PRD_STOP(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x14))
#define	I2C_PRD_DATA(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x18))

#define I2C_CFG0(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x80))
#define	I2C_MASK_CFG0_RX_FIFO_UNDER	(1 << 7)
#define	I2C_MASK_CFG0_RX_FIFO_OVER	(1 << 6)
#define	I2C_MASK_CFG0_TX_FIFO_UNDER	(1 << 5)
#define	I2C_MASK_CFG0_TX_FIFO_OVER	(1 << 4)
#define	I2C_MASK_CFG0_RX_FIFO_CLR	(1 << 3)
#define	I2C_MASK_CFG0_TX_FIFO_CLR	(1 << 2)
#define	I2C_MASK_CFG0_RX_DMA_EN		(1 << 1)
#define	I2C_MASK_CFG0_TX_DMA_EN		(1 << 0)

#define I2C_CFG1(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x84))
#define I2C_MASK_CFG1_RX_FIFO_TH	(1 << 24)
#define I2C_MASK_CFG1_TX_FIFO_TH	(1 << 16)
#define I2C_MASK_CFG1_RX_FIFO_CNT	(3 << 8)
#define I2C_MASK_CFG1_TX_FIFO_CNT	(3 << 0)

#define I2C_WDATA(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x88))
#define I2C_RDATA(i2c) \
	(*(volatile uint32_t *) (I2C_BASE(i2c) + 0x8c))

#define	I2C_ADD(field, value) \
	(I2C_MASK_##field & ((value) << (ffs(I2C_MASK_##field) - 1)))
#define I2C_DEL(field) (~I2C_MASK_##field)
#define I2C_GET(field, reg) \
	(((I2C_MASK_##field) & (reg)) >> (ffs(I2C_MASK_##field) - 1))

#define	XCLK_kHz	40000	/* 40 MHz XTAL clock */


static void i2c_clear_fifos(unsigned i2c)
{
	I2C_CFG0(i2c) = I2C_ADD(CFG0_RX_FIFO_CLR, 1) |
	    I2C_ADD(CFG0_TX_FIFO_CLR, 1);
}


static void i2c_setup(unsigned i2c, unsigned addr, unsigned reg, bool rd,
    unsigned len)
{
	uint32_t cfg;

	/* clear fifos */
//	i2c_clear_fifos(i2c);
	I2C_INT_STS(i2c) |= I2C_MASK_INT_END_CLR;

//	assert(len);
	I2C_SUB_ADDR(i2c) = reg;
	cfg = I2C_CONFIG(i2c);
	cfg &= I2C_DEL(CFG_PKT_LEN) & I2C_DEL(CFG_SLV_ADDR) &
	    I2C_DEL(CFG_SUB_ADDR_LEN) & I2C_DEL(CFG_SUB_ADDR_EN) &
	    I2C_DEL(CFG_10B_EN) & I2C_DEL(CFG_M_EN) & I2C_DEL(CFG_RnW);
	cfg |= I2C_ADD(CFG_PKT_LEN, len ? len - 1 : 0) |
	    I2C_ADD(CFG_SLV_ADDR, addr) |
	    I2C_ADD(CFG_SUB_ADDR_LEN, I2C_SUB_ADDR_LEN_1) |
	    I2C_ADD(CFG_SUB_ADDR_EN, len ? 1 : 0) |
	    I2C_ADD(CFG_SCL_SYNC_EN, 1) |
	    I2C_ADD(CFG_DEG_EN, 1) | I2C_ADD(CFG_RnW, rd);
	I2C_CONFIG(i2c) = cfg;
//debug("CFG(0) 0x%08x\n", cfg);
	cfg |= I2C_ADD(CFG_M_EN, 1);
	I2C_CONFIG(i2c) = cfg;
//debug("CFG(1) 0x%08x\n", cfg);
}


static void i2c_disable(unsigned i2c)
{
	I2C_CONFIG(i2c) &= I2C_DEL(CFG_M_EN);
}


static void i2c_end(unsigned i2c)
{
	while (!(I2C_INT_STS(i2c) & I2C_MASK_INT_END));
	i2c_disable(i2c);
}


unsigned i2c_write(unsigned i2c, unsigned addr, unsigned reg,
    const void *data, unsigned len)
{
	const uint8_t *d = data;
	unsigned left = 0;

	i2c_clear_fifos(i2c);

	// @@@ the I2C IP doesn't seem to like not having data in the FIFO when
	// it is ready to fetch something. instead, it outputs old data, and
	// can even change the message length. very confusing. for now, we just
	// pre-load the FIFO, but this only works for a few bytes.
//	i2c_setup(i2c, addr, reg, 0, len);

	// @@@ BL808 I2C doesn't let us send messages with no data. So instead,
	// we make a message without sub-address ("register"), and send the
	// sub-address as payload byte. Maybe we should just not use the
	// sub-address feature ?
	if (!len) {
		data = &reg;
		len++;
	}
	while (1) {
		uint32_t word = d[0];

		switch (left) {
		default:
			word |= d[3] << 24;
			/* fall through */
		case 3:
			word |= d[2] << 16;
			/* fall through */
		case 2:
			word |= d[1] << 8;
			/* fall through */
		case 1:
			break;
		}
		while (!I2C_GET(CFG1_TX_FIFO_CNT, I2C_CFG1(0)));
//debug("  %08x\n", word);
		I2C_WDATA(i2c) = word;
		if (left <= 4)
			break;
		left -= 4;
		d += 4;
	}

	// @@@ undo zero-data hack
	if (data == &reg)
		len--;

	i2c_setup(i2c, addr, reg, 0, len);
	i2c_end(i2c);
// @@@ we still seem to need this :-( else, transmission may get garbled
	mdelay(1);
	return len;
}


bool i2c_read(unsigned i2c, unsigned addr, unsigned reg,
    void *data, unsigned len)
{
	uint8_t *d = data;

//debug("F0: 0x%08x\n", I2C_CFG1(0));
	i2c_setup(i2c, addr, reg, 1, len);
	while (1) {
		uint32_t word;

//debug("F1: 0x%08x\n", I2C_CFG1(0));
		while (!I2C_GET(CFG1_RX_FIFO_CNT, I2C_CFG1(0)));
		word = I2C_RDATA(i2c);
//debug("D: 0x%08x\n", word);

		switch (len) {
		default:
			d[3] = word >> 24;
			/* fall through */
		case 3:
			d[2] = word >> 16;
			/* fall through */
		case 2:
			d[1] = word >> 8;
			/* fall through */
		case 1:
			d[0] = word;
			break;
		}
		if (len <= 4)
			break;
		len -= 4;
		d += 4;
	}
	i2c_end(i2c);
	return 1;
}


void i2c_init(unsigned i2c, unsigned sda, unsigned scl, unsigned kHz)
{
	/* SDA and SCL share the same GPIO_FN codepoint */
	static const uint8_t fn[] = { GPIO_FN_I2C0_SDA, GPIO_FN_I2C1_SDA,
	    GPIO_FN_I2C2_SDA, GPIO_FN_I2C3_SDA };
	unsigned phase;

	/*
	 * @@@ copy fancy adjustments from
	 * bouffalo_sdk/drivers/lhal/src/bflb_i2c.c:bflb_i2c_set_frequence
	 * ?
	 */
	phase = (XCLK_kHz / kHz / 4) - 1;
	phase = phase > 255 ? 255 : phase ? phase : 1;
	I2C_PRD_START(i2c) = I2C_PRD_STOP(i2c) = I2C_PRD_DATA(i2c) =
	    0x1010101 * phase;

	gpio_cfg_out(sda, 1, 3);
	gpio_cfg_out(scl, 1, 3);
	gpio_cfg_in(sda, GPIO_PULL_UP);
	gpio_cfg_in(scl, GPIO_PULL_UP);
	gpio_cfg_fn(sda, fn[i2c]);
	gpio_cfg_fn(scl, fn[i2c]);

	/* clean up in case we got interrupted before */

	i2c_disable(i2c);
	I2C_BUS_BUSY(i2c) = I2C_MASK_BUS_BUSY_CLR;
	i2c_clear_fifos(i2c);
}
