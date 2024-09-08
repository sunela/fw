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
#include <string.h>
#include <assert.h>

#include "rnd.h"
#include "block.h"
#include "dbcrypt.h"


bool db_encrypt(const struct dbcrypt *c, void *block, const void *content,
    unsigned length)
{
	struct block *b = (void *) block;
	struct block_content *bc = &b->content;

	assert(length == sizeof(struct block_content));
	rnd_bytes(b->nonce, sizeof(b->nonce));
	memcpy(bc, content, sizeof(struct block_content));
	memset(b->hash, 0, sizeof(b->hash));
	return 1;
}


int db_decrypt(const struct dbcrypt *c, void *content, unsigned size,
    const void *block)
{
	const struct block *b = (const void *) block;
	const struct block_content *bc = &b->content;

	assert(size == sizeof(*bc));
	memcpy(content, bc, sizeof(*bc));
	return sizeof(*bc);
}
