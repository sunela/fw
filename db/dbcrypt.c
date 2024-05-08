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

#include "block.h"
#include "dbcrypt.h"


bool db_encrypt(const struct dbcrypt *c, void *block, const void *content)
{
	struct block *b = (void *) block;
	struct block_content *bc = &b->content;

	memset(b->nonce, 0, sizeof(b->nonce));
	memcpy(bc, content, sizeof(struct block_content));
	memset(b->hash, 0, sizeof(b->hash));
	return 1;
}


bool db_decrypt(const struct dbcrypt *c, void *content, const void *block)
{
	const struct block *b = (const void *) block;
	const struct block_content *bc = &b->content;

	memcpy(content, bc, sizeof(*bc));
	return 1;
}
