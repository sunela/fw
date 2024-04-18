/*
 * sha.h - Driver for BL808 SHA hardware accelerator
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * For padding, etc.:
 * https://en.wikipedia.org/wiki/SHA-1
 *
 * The original specification:
 * https://nvlpubs.nist.gov/nistpubs/Legacy/FIPS/fipspub180-1.pdf
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include "stdio.h"

#include "mmio.h"
#include "physmem.h"
#include "sha.h"


#define	SHA_BASE       		(mmio_m0_base + 0x4000)
#define	SHA_CTRL		(*(volatile uint32_t *) SHA_BASE)
#define	SHA_MASK_CTRL_MSG_LEN	(0xff << 16)	// in 512-bit blocks
#define	SHA_MASK_LINK_MODE		(1 << 15)
#define	SHA_MASK_MODE_EXT		(3 << 12)
enum SHA_MODE_EXT {
	SHA_MODE_EXT_SHA		= 0,
	SHA_MODE_EXT_MD5		= 1,
	SHA_MODE_EXT_CRC16		= 2,
	SHA_MODE_EXT_CRC32		= 3,
};
#define	SHA_MASK_INT_MASK		(1 << 11)
#define	SHA_MASK_INT_SET		(1 << 10)
#define	SHA_MASK_INT_CLR		(1 << 9)
#define	SHA_MASK_INT			(1 << 8)
#define	SHA_MASK_HASH_SEL		(1 << 6)
#define	SHA_MASK_EN			(1 << 5)
#define	SHA_MASK_MODE			(7 << 2)
enum SHA_MODE {
	SHA_MODE_SHA256			= 0,
	SHA_MODE_SHA224			= 1,
	SHA_MODE_SHA1			= 2,	// also 3
	SHA_MODE_SHA512			= 4,
	SHA_MODE_SHA384			= 5,
	SHA_MODE_SHA512_224		= 6,
	SHA_MODE_SHA512_256		= 7,
};
#define	SHA_MASK_TRIG			(1 << 1)
#define	SHA_MASK_BUSY			(1 << 0)
#define	SHA_MSA			(*(volatile uint32_t *) (SHA_BASE + 0x4))
#define	SHA_HASH_I(n)		(*(volatile uint32_t *) \
				    (SHA_BASE + 0x10 + 4 * (n)))

#define SHA_ADD(field, value) \
	(SHA_MASK_##field & ((value) << (ffs(SHA_MASK_##field) - 1)))
#define SHA_DEL(field)	(~SHA_MASK_##field)


static volatile void *buf = NULL;
static unsigned long buf_paddr;
static unsigned got = 0;
static uint64_t length = 0;

//#define	DEBUG

#ifdef DEBUG
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define	PAGE_SIZE	sysconf(_SC_PAGE_SIZE)

static volatile uint8_t *check;


static void *peek(unsigned long paddr)
{
	void *base;
	int fd;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("/dev/mem");
		exit(1);
	}
        base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
	    paddr);
	if (base == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	return base;
}
#endif


void sha1_begin(void)
{
	uint32_t ctrl;

	if (!buf) {
		void **bufs;
		unsigned long *paddrs;

		bufs = calloc_phys_vec(1, SHA1_BLOCK_BYTES);
		paddrs = xlat_virt(bufs, 1);
		buf = *bufs;
		buf_paddr = *paddrs;
		free(bufs);
		free(paddrs);
	}
	ctrl = SHA_CTRL;
	ctrl &= SHA_DEL(MODE_EXT) & SHA_DEL(MODE) & SHA_DEL(CTRL_MSG_LEN) &
	    ~SHA_MASK_HASH_SEL & ~SHA_MASK_EN & ~SHA_MASK_TRIG;
	ctrl |= SHA_ADD(MODE_EXT, SHA_MODE_EXT_SHA) | SHA_ADD(CTRL_MSG_LEN, 1) |
	    SHA_ADD(MODE, SHA_MODE_SHA1);
	SHA_CTRL = ctrl;
	SHA_CTRL |= SHA_MASK_EN;
//printf("virt %p phys 0x%llx\n", buf, (unsigned long long) buf_paddr);
	SHA_MSA = buf_paddr;
	got = 0;
#ifdef DEBUG
check = peek(buf_paddr);
#endif
	length = 0;
}


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

static void flush_cache(void)
{
	asm volatile ("fence.i" ::: "memory");
}


void sha1_hash(const uint8_t *data, size_t size)
{
	assert(got < SHA1_BLOCK_BYTES);
	length += size;
	while (size) {
		size_t this;

		this = got + size <= SHA1_BLOCK_BYTES ? size :
		    SHA1_BLOCK_BYTES - got;
		memcpy((void *) buf + got, data, this);
		flush_cache();
		got += this;
		data += this;
		size -= this;
		if (got == SHA1_BLOCK_BYTES) {
#ifdef DEBUG
unsigned i;

printf("V");
for (i = 0; i != SHA1_BLOCK_BYTES; i++)
	printf(" %02x", ((uint8_t *) buf)[i]);
printf("\n");
printf("P");
for (i = 0; i != SHA1_BLOCK_BYTES; i++)
	printf(" %02x", check[i]);
printf("\n");
#endif
			SHA_CTRL |= SHA_MASK_TRIG;
			while (SHA_CTRL & SHA_MASK_BUSY);
			SHA_CTRL |= SHA_MASK_HASH_SEL;
			got = 0;
		}
	}
}


void sha1_end(uint8_t res[SHA1_HASH_BYTES])
{
	static const uint8_t bit1 = 0x80;
	static const uint8_t zero[SHA1_BLOCK_BYTES] = { 0, };
	uint64_t bits = length << 3;
	unsigned i;

	sha1_hash(&bit1, 1);
	sha1_hash(zero,
	    (2 * SHA1_BLOCK_BYTES - 8 - got) % SHA1_BLOCK_BYTES);
	for (i = 0; i != 8; i++) {
		uint8_t tmp = bits >> (56 - i * 8);

		sha1_hash(&tmp, 1);
	}
	assert(!got);
	for (i = 0; i != SHA1_HASH_BYTES; i++)
		res[i] = SHA_HASH_I(i >> 2) >> ((i & 3) << 3);
	SHA_CTRL &= ~SHA_MASK_EN & ~SHA_MASK_HASH_SEL;
//	SHA_CTRL &= ~SHA_MASK_EN;
}
