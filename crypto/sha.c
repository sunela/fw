/*
 * sha.h - SHA1 and SHA256 using GNU Crypto
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


/* --- SHA1 ---------------------------------------------------------------- */


static bool sha1_initialized = 0;
static gcry_md_hd_t sha1;


static void sha1_init(void)
{
	gcry_error_t err;

	if (sha1_initialized)
		return;
	gcry_check_version(NULL);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
	err = gcry_md_open(&sha1, GCRY_MD_SHA1, 0);
	if (err) {
		debug("gcry_md_open: %s\n", gcry_strerror(err));
		exit(1);
	}
	sha1_initialized = 1;
}


void sha1_begin(void)
{
	sha1_init();
	gcry_md_reset(sha1);
}


void sha1_hash(const uint8_t *data, size_t size)
{
	gcry_md_write(sha1, data, size);
}


void sha1_end(uint8_t res[SHA1_HASH_BYTES])
{
	unsigned char *hash;

	hash = gcry_md_read(sha1, GCRY_MD_SHA1);
	memcpy(res, hash, SHA1_HASH_BYTES);
}


/* --- SHA256 -------------------------------------------------------------- */


static bool sha256_initialized = 0;
static gcry_md_hd_t sha256;


static void sha256_init(void)
{
	gcry_error_t err;

	if (sha256_initialized)
		return;
	gcry_check_version(NULL);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
	err = gcry_md_open(&sha256, GCRY_MD_SHA256, 0);
	if (err) {
		debug("gcry_md_open: %s\n", gcry_strerror(err));
		exit(1);
	}
	sha256_initialized = 1;
}


void sha256_begin(void)
{
	sha256_init();
	gcry_md_reset(sha256);
}


void sha256_hash(const uint8_t *data, size_t size)
{
	gcry_md_write(sha256, data, size);
}


void sha256_end(uint8_t res[SHA256_HASH_BYTES])
{
	unsigned char *hash;

	hash = gcry_md_read(sha256, GCRY_MD_SHA256);
	memcpy(res, hash, SHA256_HASH_BYTES);
}
