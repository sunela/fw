/*
 * block.h - Block-level operations
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef BLOCK_H
#define BLOCK_H

#include <stdbool.h>
#include <stdint.h>

#include "sha.h"
#include "dbcrypt.h"


/*
 * Block structure:
 *
 * Offset
 * |	Size
 * 0	1	Status (0, 0xff: unused, 1: used, 2: invalid,
 *		all other values are reserved
 * 1	7	Reserved
 * 8	24	Nonce
 * 32	992	Encrypted content
 *
 * Encrypted content:
 *
 * Offset
 * |	Size
 * 0	1	Block type (empty, single, first, middle, last)
 * 1	3	Reserved
 * 4	956	Payload
 * 960	20	Hash (SHA-1)
 * 980	12	Reserved
 */

#define	BLOCK_PAYLOAD_SIZE	956


enum block_status {
	bs_unallocated_1	= 0xff,
	bs_unallocated_2	= 0,
	bs_allocated		= 1,
	bs_invalidated		= 2,
};

enum content_type {
	ct_empty		= 3,	/* the block is allocated but does not
					   contain valid data */
	ct_single		= 4,	/* only block in a sequence */
	ct_last			= 5,	/* last block in a sequence */
	/* 6 - 14 are reserved */
	/* 15 is used for bt_corrupt */
	ct_first		= 16,	/* block #0 */
	ct_nth			= 17,	/* block #n, 0-based -> c_first + n */
	ct_max			= 0xef,	/* up to 240 blocks per sequence */
	/* 0xf0 - 0xff are reserved */
};

/*
 * "enum block_type" merges information from "enum block_status" and "enum
 * content_type", We use "enum block_status" and "enum content_type" instead of
 * just "enum block_type" for everything to make it clear which values are
 * used at each level.
 */

enum block_type {
	bt_error	= -1,	/* IO error, or similar */
	bt_unallocated	= bs_unallocated_2,
	bt_corrupt	= 15,	/* damaged content */
	bt_invalidated	= bs_invalidated,
	bt_empty	= ct_empty,
	bt_single	= ct_single,
	bt_last		= ct_last,
	bt_first	= ct_first,
	bt_nth		= ct_nth,
	bt_max		= ct_max,
};

struct block {
	uint8_t	status;
	uint8_t	reserved[7];
	uint8_t	nonce[DB_NONCE_SIZE];
	struct block_content {
		uint8_t type;
		uint8_t	reserved_1[3];	/* set to zero */
		uint8_t payload[BLOCK_PAYLOAD_SIZE];
	} content;
	uint8_t hash[SHA1_HASH_BYTES];
	uint8_t reserved_2[12];	/* set to zero */
};



/*
 * block_read returns the decrypted payload in the buffer at "payload".
 * If the block type is bt_error, bt_invalid, or bt_empty, no payload data is
 * returned.
 */
enum block_type block_read(const struct dbcrypt *c, void *payload, unsigned n);

/*
 * block_write can create the block types all block types, except bt_error and
 * bt_corrupt.
 */
bool block_write(const struct dbcrypt *c, enum block_type type,
    const void *buf, unsigned n);

bool block_invalidate(unsigned n);
bool block_deallocate(unsigned n);

#endif /* !BLOCK_H */
