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


static enum block_type classify_block(const uint8_t *b)
{
	unsigned i;

	switch (*b++) {
	case 0:
		for (i = 1; i != DB_NONCE_SIZE; i++)
			if (*b++)
				return bt_data;
		return bt_deleted;
	case 0xff:
		for (i = 1; i != DB_NONCE_SIZE; i++)
			if (*b++ != 0xff)
				return bt_data;
		while (i++ != STORAGE_BLOCK_SIZE)
			if (*b++ != 0xff)
				return bt_invalid;
		return bt_erased;
	default:
		return bt_data;	/* or bt_empty or bt_invalid */
	}
}


enum block_type block_read(const struct dbcrypt *c, void *payload, unsigned n)
{
	enum block_type type;

debug("block_read %u\n", n);
	if (!storage_read_block(io_buf, n))
		return bt_error;
	type = classify_block((const uint8_t *) io_buf);
	switch (type) {
	case bt_deleted:
	case bt_erased:
	case bt_invalid:
		return type;
	default:
		break;
	}
	if (!db_decrypt(c, &bc, io_buf)) {
		memset(&bc, 0, sizeof(bc));
		return bt_invalid;
	}
	type = bc.type;
	switch (bc.type) {
	case bt_data:
		memcpy(payload, bc.payload, sizeof(bc.payload));
		break;
	case bt_empty:
		break;
	default:
		type = bt_invalid;
		break;
	}
	memset(&bc, 0, sizeof(bc));
	return type;
}


bool block_write(const struct dbcrypt *c, enum content_type type,
    const void *payload, unsigned n)
{
	assert(sizeof(struct block) == STORAGE_BLOCK_SIZE);
	memset(io_buf, 0, sizeof(io_buf));
	bc.type = type;
	switch (type) {
	case bt_empty:
		memset(&bc, 0, sizeof(bc));
		break;
	case bt_data:
		memcpy(bc.payload, payload, sizeof(bc.payload));
		break;
	default:
		abort();
	}
	if (!db_encrypt(c, io_buf, &bc))
		return 0;
	memset(&bc, 0, sizeof(bc));
	return storage_write_block(io_buf, n);
}


bool block_delete(unsigned n)
{
	memset(io_buf, 0, sizeof(io_buf));
	return storage_write_block(io_buf, n);
}
