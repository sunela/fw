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


static PSRAM_NOINIT uint8_t io_buf[STORAGE_BLOCK_SIZE];
static PSRAM_NOINIT uint8_t bc[STORAGE_BLOCK_SIZE];
	// @@@ beyond-worst-case size


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


enum block_type block_read(const struct dbcrypt *c, uint16_t *seq,
    void *payload, unsigned *payload_len, unsigned n)
{
	const struct block_header *hdr = (const void *) bc;
	enum block_type type;
	int got;

	if (!storage_read_block(io_buf, n))
		return bt_error;
	type = classify_block((const uint8_t *) io_buf);
	if (!payload)
		return type;
	switch (type) {
	case bt_deleted:
	case bt_erased:
	case bt_invalid:
		return type;
	default:
		break;
	}
	got = db_decrypt(c, bc, sizeof(bc), io_buf);
	if (got < 0) {
		memset(bc, 0, sizeof(bc));
		return bt_invalid;
	}
	assert((unsigned) got >= sizeof(*hdr));
	assert((unsigned) got <= sizeof(*hdr) + *payload_len);
	type = hdr->type;
	switch (hdr->type) {
	case bt_data:
		if (seq)
			*seq = hdr->seq;
		memcpy(payload, bc + sizeof(*hdr), got - sizeof(*hdr));
		*payload_len = got - sizeof(*hdr);
		break;
	case bt_empty:
		*payload_len = 0;
		break;
	default:
		type = bt_invalid;
		*payload_len = 0;
		break;
	}
	memset(bc, 0, sizeof(bc));
	return type;
}


bool block_write(const struct dbcrypt *c, enum content_type type, uint16_t seq,
    const void *payload, unsigned length, unsigned n)
{
	struct block_header *hdr = (void *) bc;

	assert(sizeof(*hdr) + length <= sizeof(bc));
	memset(io_buf, 0, sizeof(io_buf));
	memset(hdr, 0, sizeof(*hdr));
	hdr->type = type;
	hdr->seq = seq;
	switch (type) {
	case bt_empty:
		memset(bc, 0, sizeof(bc));
		break;
	case bt_data:
		memcpy(bc + sizeof(*hdr), payload, length);
		break;
	default:
		abort();
	}
	if (!db_encrypt(c, io_buf, bc, sizeof(*hdr) + length))
		return 0;
	memset(bc, 0, sizeof(bc));
	return storage_write_block(io_buf, n);
}


bool block_delete(unsigned n)
{
	memset(io_buf, 0, sizeof(io_buf));
	return storage_write_block(io_buf, n);
}
