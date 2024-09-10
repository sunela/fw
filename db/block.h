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
 * 0	24	Nonce (0 if ct_empty)
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

struct block_header {
	uint8_t type;
	uint8_t	reserved;	/* set to zero */
	uint16_t seq;		/* sequence number */
};

struct block_content {
	struct block_header hdr;
	uint8_t payload[BLOCK_PAYLOAD_SIZE];
};



/*
 * block_read returns the decrypted payload in the buffer at "payload".
 * When calling block_read, payload_len points to the size of the buffer. After
 * a successful read, the length of the decrypted payload is stored in
 * *payload_len.
 *
 * If the block type is anything other than bt_data, neither sequence number
 * nor payload data are returned. If the sequence number is not needed, a NULL
 * pointer can be passed for "seq".
 */
enum block_type block_read(const struct dbcrypt *c, uint16_t *seq,
    void *payload, unsigned *payload_len, unsigned n);

/*
 * block_write requires the block to be erased. Note that attempting to write
 * to a block that is not completelly erased is likely to produce an invalid
 * block, losing any (valid) data that may have been stored there before.
 */
bool block_write(const struct dbcrypt *c, enum content_type type, uint16_t seq,
    const void *payload, unsigned length, unsigned n);

bool block_delete(unsigned n);

#endif /* !BLOCK_H */
