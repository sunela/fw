/*
 * block.c - Block-level operations 
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "sha.h"
#include "dbcrypt.h"
#include "storage.h"
#include "block.h"

#include "debug.h"


static PSRAM uint8_t io_buf[STORAGE_BLOCK_SIZE];
static PSRAM struct block_content bc;



enum block_type block_read(const struct dbcrypt *c, void *payload, unsigned n)
{
	const struct block *b = (const void *) io_buf;
	enum block_type ret;

debug("block_read %u\n", n);
	if (!storage_read_block(io_buf, n))
		return bt_error;
debug("\tstatus %u\n", b->status);
	switch (b->status) {
	case bs_unallocated_1:
	case bs_unallocated_2:
		return bt_unallocated;
	case bs_allocated:
		break;
	case bs_invalidated:
		return bt_invalidated;
	default:
		abort();
	}
	if (!db_decrypt(c, &bc, io_buf)) {
		memset(&bc, 0, sizeof(bc));
		return bt_corrupt;
	}
	switch (bc.type) {
	case bt_single:
	case bt_first:
	case bt_nth ... bt_max:
	case bt_last:
		memcpy(payload, bc.payload, sizeof(bc.payload));
		/* fall through */
	case bt_empty:
		ret = bc.type;
		break;
	default:
		ret = bt_corrupt;
	}
	memset(&bc, 0, sizeof(bc));
	return ret;
}


bool block_write(const struct dbcrypt *c, enum block_type type,
    const void *payload, unsigned n)
{
	struct block *b = (void *) io_buf;

	assert(sizeof(struct block) == STORAGE_BLOCK_SIZE);
	memset(io_buf, 0, sizeof(io_buf));
	memset(&bc, 0, sizeof(bc));
	switch (type) {
	case bt_invalidated:
		return block_invalidate(n);
	case bt_single:
	case bt_first:
	case bt_nth ... bt_max:
	case bt_last:
		memcpy(bc.payload, payload, sizeof(bc.payload));
		break;
	case bt_empty:
		break;
	default:
		abort();
	}
	b->status = bs_allocated;
	bc.type = type;
	if (!db_encrypt(c, io_buf, &bc))
		return 0;
	memset(&bc, 0, sizeof(bc));
	return storage_write_block(io_buf, n);
}


bool block_invalidate(unsigned n)
{
	struct block *b = (void *) io_buf;

	memset(io_buf, 0, sizeof(io_buf));
	b->status = bs_invalidated;
	return storage_write_block(io_buf, n);
}


bool block_deallocate(unsigned n)
{
	struct block *b = (void *) io_buf;

	memset(io_buf, 0, sizeof(io_buf));
	b->status = bs_unallocated_1;
	return storage_write_block(io_buf, n);
}
