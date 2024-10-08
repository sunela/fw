/*
 * dbcrypt.c - Database encryption
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ For now, we neither encrypt nor do we check the hash.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "alloc.h"
#include "rnd.h"
#include "tweetnacl.h"
#include "storage.h"
#include "block.h"
#include "dbcrypt.h"

#if 1
#define	t0()	do {} while (0)
#define	t1(...)	do {} while (0)
#endif


struct peer {
	uint8_t k[crypto_box_BEFORENMBYTES];
	uint8_t pk[crypto_box_PUBLICKEYBYTES];
	struct peer *next;
};

struct dbcrypt {
	uint8_t sk[crypto_box_SECRETKEYBYTES];
	uint8_t pk[crypto_box_PUBLICKEYBYTES];
	/*
	 * "readers" is the list of readers for which we include (encrypted)
	 * record keys in each block.
	 */
	struct peer *readers;
	/*
	 * "cache" are writers of blocks we decrypt, but who are not in the		 * readers list.
	 */
	struct peer *cache;
};


/*
 * Each block contains at least:
 * - the write's pubkey (Pw),
 * - a nonce,
 * - one reader. We store for each reader r:
 *   - the record key, encrypted with Shared(Sw, Pr)
 *
 * There is no information about who can decrypt the record key, nor how many
 * record keys the block contains. The reader therefor has to try all possible
 * combinations until a key fits.
 */

#define	BLOCK_OVERHEAD	(crypto_box_PUBLICKEYBYTES +		\
			    crypto_secretbox_NONCEBYTES +	\
			    crypto_secretbox_KEYBYTES)

/*
 * MAX_MLEN is the maximum message length given to crypto_secretbox and
 * crypto_secretbox_open. It is the size of the maximum payload plus the
 * required padding.
 */

#define	MAX_MLEN	(STORAGE_BLOCK_SIZE + crypto_secretbox_ZEROBYTES - \
			    BLOCK_OVERHEAD)

/*
 * secretbox bytes addition to the encrypted payload
 */

#define	BOX_OVERHEAD	(crypto_secretbox_ZEROBYTES -		\
			    crypto_secretbox_BOXZEROBYTES)

/*
 * NaCl's C API applies padding to its input and output buffers, so we can't
 * operate directly on block and plaintext buffers (which have no room for the
 * padding), but have to go through intermediary buffers.
 *
 * @@@ note: we keep these buffers in chip RAM for now. could they safely go to
 * pSRAM ?
 */

static uint8_t in_buf[STORAGE_BLOCK_SIZE + crypto_secretbox_ZEROBYTES];
static uint8_t out_buf[STORAGE_BLOCK_SIZE + crypto_secretbox_ZEROBYTES];


/* --- Encrypt ------------------------------------------------------------- */


static void db_encrypt_payload(void *block, uint8_t *encrypted,
    const void *content, unsigned length, const uint8_t *rk)
{
	const uint8_t *block_end = block + STORAGE_BLOCK_SIZE;
	uint8_t *nonce = block + crypto_box_PUBLICKEYBYTES;
	unsigned encrypted_bytes = block_end - encrypted;
	unsigned mlen = crypto_secretbox_ZEROBYTES - BOX_OVERHEAD +
	    encrypted_bytes;

	assert(mlen <= sizeof(in_buf));
	assert(mlen <= sizeof(out_buf));
	memset(in_buf, 0, sizeof(in_buf));
	memcpy(in_buf + crypto_secretbox_ZEROBYTES, content, length);

	t0();
	if (crypto_secretbox(out_buf, in_buf, mlen, nonce, rk))
		DIE("crypto_secretbox failed");
	t1("db_encrypt:crypto_secretbox\n");

	memcpy(encrypted, out_buf + crypto_secretbox_BOXZEROBYTES,
	    mlen - crypto_secretbox_BOXZEROBYTES);
	assert(encrypted + mlen - crypto_secretbox_BOXZEROBYTES == block_end);
}


bool db_encrypt(const struct dbcrypt *c, void *block, const void *content,
    unsigned length)
{
	/* --- block layout --- */

	const struct peer *reader;
	unsigned n_readers = 0;

	for (reader = c->readers; reader; reader = reader->next)
		n_readers++;
	assert(n_readers);

	const uint8_t *block_end = block + STORAGE_BLOCK_SIZE;
	uint8_t *wpk = block;	/* writer's pubkey */
	uint8_t *nonce = wpk + crypto_box_PUBLICKEYBYTES;
	uint8_t *reader_list = nonce + crypto_secretbox_NONCEBYTES;
	unsigned reader_list_bytes = n_readers * crypto_secretbox_KEYBYTES;
	uint8_t *encrypted = reader_list + reader_list_bytes;
	unsigned encrypted_bytes = block_end - encrypted;

	assert(encrypted + BOX_OVERHEAD <= block_end);
	assert(encrypted_bytes >= length + BOX_OVERHEAD);

	/* --- generate nonce and record key --- */

	uint8_t rk[crypto_secretbox_KEYBYTES];

	rnd_bytes(nonce, crypto_secretbox_NONCEBYTES);
	rnd_bytes(rk, crypto_secretbox_KEYBYTES);

	/* --- encrypt --- */

	db_encrypt_payload(block, encrypted, content, length, rk);

	/* --- populate the rest of the block --- */

	memcpy(wpk, c->pk, crypto_box_PUBLICKEYBYTES);

	uint8_t *b = reader_list;
	uint8_t nonce2[crypto_secretbox_NONCEBYTES];
	uint8_t i = 0;

	for (reader = c->readers; reader; reader = reader->next) {
		memset(in_buf, 0, crypto_secretbox_ZEROBYTES);
		memcpy(in_buf + crypto_secretbox_ZEROBYTES, rk,
		    crypto_secretbox_KEYBYTES);
		memcpy(nonce2, nonce, crypto_secretbox_NONCEBYTES);
		i++;
		nonce2[0] ^= i;

		t0();
		if (crypto_stream_xor(out_buf, in_buf,
		    crypto_secretbox_ZEROBYTES + crypto_secretbox_KEYBYTES,
		    nonce2, reader->k))
			DIE("crypto_stream_xor failed");
		t1("db_encrypt:crypto_stream_xor\n");

		memcpy(b, out_buf + crypto_secretbox_ZEROBYTES,
		    crypto_secretbox_KEYBYTES);
		b += crypto_secretbox_KEYBYTES;
	}
	assert(b == encrypted);

	/* --- clean up --- */

	memset(rk, 0, sizeof(rk));
	memset(in_buf, 0, sizeof(in_buf));
	memset(out_buf, 0, sizeof(out_buf));

	return 1;
}


/* --- Decrypt ------------------------------------------------------------- */


static int db_decrypt_payload(void *content, unsigned size, const void *block,
    const uint8_t *encrypted, const uint8_t *rk)
{
	int length = -1; /* -1 means that we could not decrypt the block */
	const uint8_t *block_end = block + STORAGE_BLOCK_SIZE;
	const uint8_t *nonce = block + crypto_box_PUBLICKEYBYTES;
	unsigned encrypted_bytes = block_end - encrypted;

	/* --- decrypt the payload --- */

	unsigned mlen = crypto_secretbox_BOXZEROBYTES + encrypted_bytes;

	assert(mlen <= sizeof(in_buf));
	assert(mlen <= sizeof(out_buf));
	memset(in_buf, 0, crypto_secretbox_BOXZEROBYTES);
	memcpy(in_buf + crypto_secretbox_BOXZEROBYTES, encrypted,
	    encrypted_bytes);

	t0();
	if (crypto_secretbox_open(out_buf, in_buf, mlen, nonce, rk)) {
		debug("crypto_secretbox failed\n");
		goto cleanup;
	}
	t1("db_decrypt:crypto_secretbox_open\n");

	/* --- extract the payload --- */

	length = mlen - crypto_secretbox_ZEROBYTES;
	assert(mlen >= crypto_secretbox_ZEROBYTES);
	assert((unsigned) length <= size);
	memcpy(content, out_buf + crypto_secretbox_ZEROBYTES, length);

	/* --- clean up --- */

cleanup:
	memset(in_buf, 0, sizeof(in_buf));
	memset(out_buf, 0, sizeof(out_buf));

	return length;
}


static int db_try_decrypt(void *content, unsigned size, const void *block,
    const uint8_t *encrypted, unsigned i, const uint8_t *ek,
    const uint8_t *shared)
{
	int length = -1; /* means that we could not decrypt the block */
	const uint8_t *nonce = block + crypto_box_PUBLICKEYBYTES;
	uint8_t rk[crypto_box_PUBLICKEYBYTES];
	uint8_t nonce2[crypto_secretbox_NONCEBYTES];

	/* --- decrypt the record key --- */

	memcpy(nonce2, nonce, crypto_secretbox_NONCEBYTES);
	nonce2[0] ^= i + 1;
	memset(in_buf, 0, crypto_secretbox_ZEROBYTES);
	memcpy(in_buf + crypto_secretbox_ZEROBYTES, ek,
	    crypto_secretbox_KEYBYTES);

	t0();
	if (crypto_stream_xor(out_buf, in_buf,
	    crypto_secretbox_ZEROBYTES + crypto_secretbox_KEYBYTES, nonce2,
	    shared)) {
		debug("crypto_stream_xor failed\b");
		goto cleanup;
	}
	t1("db_decrypt:crypto_stream_xor\n");

	memcpy(rk, out_buf + crypto_secretbox_ZEROBYTES,
	    crypto_secretbox_KEYBYTES);

	/* --- decrypt the payload --- */

	length = db_decrypt_payload(content, size, block, encrypted, rk);

	/* --- clean up --- */

cleanup:
	memset(rk, 0, sizeof(rk));

	return length;
}


int db_decrypt(const struct dbcrypt *c, void *content, unsigned size,
    const void *block)
{
	int length = -1; /* means that we could not decrypt the block */

	/* --- block layout --- */

	const uint8_t *block_end = block + STORAGE_BLOCK_SIZE;
	const uint8_t *wpk = block;	/* writer's pubkey */
	const uint8_t *nonce = wpk + crypto_box_PUBLICKEYBYTES;
	const uint8_t *reader_list = nonce + crypto_secretbox_NONCEBYTES;
	unsigned n_readers;

	/* --- shared public-key encryption secret --- */

	uint8_t shared[crypto_secretbox_KEYBYTES];

	t0();
	if (crypto_box_beforenm(shared, wpk, c->sk)) {
		debug("crypto_box_beforenm failed\n");
		return -1;
	}
	t1("db_decrypt:crypto_box_beforenm\n");

	/* --- try all possible layouts and record keys --- */

	for (n_readers = 1; n_readers <= DB_MAX_READERS; n_readers++) {
		unsigned reader_list_bytes =
		    n_readers * crypto_secretbox_KEYBYTES;
		const uint8_t *encrypted = reader_list + reader_list_bytes;
		unsigned encrypted_bytes = block_end - encrypted;

		if (encrypted > block_end) {
			debug("reader list too long (%u)\n", n_readers);
			return -1;
		}
		if (encrypted_bytes < BOX_OVERHEAD) {
			debug("not enough room for box (%u < %u)\n",
			    encrypted_bytes, BOX_OVERHEAD);
			return -1;
		}

		/* --- find a suitable encrypted key --- */

		const uint8_t *b = reader_list;
		unsigned i;

		for (i = 0; i != n_readers; i++) {
			/* @@@ should first search the reader list and the
			   cache of share public-key encryption secrets  */
			/* @@@ cache decrypted record keys ? */

			/* --- decrypt the payload --- */

			length = db_try_decrypt(content,size, block, encrypted,
			    i, b, shared);

			if (length != -1) {
				debug("db_decrypt: found at %u / %u\n",
				    i, n_readers);
				goto done;
			}

			b += crypto_secretbox_KEYBYTES;
		}
	}

	/* --- clean up --- */

done:
	memset(shared, 0, sizeof(shared));

	return length;
}


/* --- Retrieve the public key --------------------------------------------- */


const void *dbcrypt_pubkey(const struct dbcrypt *c)
{
	return c->pk;
}


/* --- Manage the dbcrypt context ------------------------------------------ */


struct dbcrypt *dbcrypt_init(const void *sk, unsigned bytes)
{
	struct dbcrypt *c = alloc_type(struct dbcrypt);
	struct peer *p;

	assert(crypto_box_SECRETKEYBYTES == crypto_box_BEFORENMBYTES);
	assert(bytes == crypto_box_SECRETKEYBYTES);
	memcpy(c->sk, sk, crypto_box_SECRETKEYBYTES);

	t0();
	if (crypto_scalarmult_base(c->pk, sk))
		DIE("crypto_scalarmult_base failed");
	t1("dbcrypt_init:crypto_scalarmult_base(1)\n");

	p = alloc_type(struct peer);
	c->readers = p;
	p->next = NULL;

	t0();
	if (crypto_scalarmult_base(p->pk, sk))
		DIE("crypto_scalarmult_base failed");
	t1("dbcrypt_init:crypto_scalarmult_base(2)\n");

	t0();
	if (crypto_box_beforenm(p->k, p->pk, sk))
		DIE("crypto_box_beforenm failed");
	t1("dbcrypt_init:crypto_box_beforenm\n");

	c->cache = NULL;
	return c;
}


void dbcrypt_add_reader(struct dbcrypt *c, const void *pk, unsigned bytes)
{
	struct peer *p, **anchor;

	assert(DB_NONCE_SIZE == crypto_secretbox_NONCEBYTES);
	assert(bytes == crypto_box_PUBLICKEYBYTES);
	p = alloc_type(struct peer);
	p->next = NULL;
	for (anchor = &c->readers; *anchor; anchor = &(*anchor)->next);
	*anchor = p;
	p->next = NULL;
	memcpy(p->pk, pk, crypto_box_PUBLICKEYBYTES);

	t0();
	if (crypto_box_beforenm(p->k, p->pk, c->sk))
		DIE("crypto_box_beforenm failed");
	t1("dbcrypt_add_reader:crypto_box_beforenm\n");
}


static void free_peers(struct peer *p)
{
	while (p) {
		struct peer *next = p->next;

		memset(p, 0, sizeof(*p));
		p = next;
	}
}


void dbcrypt_free(struct dbcrypt *c)
{
	free_peers(c->readers);
	free_peers(c->cache);
	memset(c, 0, sizeof(*c));
	free(c);
}
