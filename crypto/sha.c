/*
 * sha.h - SHA1 using GNU Crypto
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <gcrypt.h>
#include <sys/types.h>

#include "debug.h"
#include "sha.h"


static bool initialized = 0;
static gcry_md_hd_t h;


static void sha1_init(void)
{
	gcry_error_t err;

	if (initialized)
		return;
	gcry_check_version(NULL);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
	err = gcry_md_open(&h, GCRY_MD_SHA1, 0);
	if (err) {
		debug("gcry_md_open: %s\n", gcry_strerror(err));
		exit(1);
	}
	initialized = 1;
}


void sha1_begin(void)
{
	sha1_init();
	gcry_md_reset(h);
}


void sha1_hash(const uint8_t *data, size_t size)
{
	gcry_md_write(h, data, size);
}


void sha1_end(uint8_t res[SHA1_HASH_BYTES])
{
	unsigned char *hash;

	hash = gcry_md_read(h, GCRY_MD_SHA1);
	memcpy(res, hash, SHA1_HASH_BYTES);
}
