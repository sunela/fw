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
 * 0	24	Nonce
 * 24	8	Reserved
 * 32	992	Encrypted content
 *  0	1	  Content type (empty, data)
 *  1	1	  Reserved
 *  2	2	  Sequence (to version IDs)
 *  4	956	  Payload (begins with NUL-terminated ID string)
 *  960	20	  Hash (SHA-1)
 *  980	12	  Reserved
 */

#define	BLOCK_PAYLOAD_SIZE	956


enum content_type {
	ct_empty		= 3,	/* the block is allocated but does not
					   contain valid data */
	ct_data			= 4,	/* only block in a sequence */
};

/*
 * "enum block_type" merges "enum content_type", physical-layer information,
 * and hash validity.
 */

enum block_type {
	bt_error	= -1,	/* IO error, or similar */
	bt_deleted	= 0,	/* nonce = 0 */
	bt_erased	= 1,	/* all bits are 1 */
	bt_invalid	= 2,	/* invalid (at least with the current key) */
	bt_empty	= ct_empty,
	bt_data		= ct_data,
};

struct block {
	uint8_t	nonce[DB_NONCE_SIZE];
	uint8_t	reserved_1[8];		/* set to zero */
	struct block_content {
		uint8_t type;
		uint8_t	reserved;	/* set to zero */
		uint16_t seq;		/* sequence number */
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
 * block_write requires the block to be erased. Note that attempting to write
 * to a block that is not completelly erased is likely to produce an invalid
 * block, losing any (valid) data that may have been stored there before.
 */
bool block_write(const struct dbcrypt *c, enum content_type type,
    const void *buf, unsigned n);

bool block_delete(unsigned n);

#endif /* !BLOCK_H */
