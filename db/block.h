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


#define	PAD_BLOCKS	8
#define	RESERVED_BLOCKS	PAD_BLOCKS

/*
 * Block structure:
 *
 * Offset
 * |	Size
 * 0	32	Writer's public key
 * 32	24	Nonce (all-zero: block is deleted)
 * 56	For each reader:
 *	32	Encrypted record key
 * Encrypted data:
 *	16	Hash
 *	*	Payload (zero-padded to fill the block)
 *	1	  Content type (empty, data)
 *	1	  Reserved (set to zero when writing, ignore when reading)
 *	2	  Sequence number (little-endian)
 */

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
	bt_settings	= 5,	/* block contains settings */
};

struct block_header {
	uint8_t type;
	uint8_t	reserved;	/* set to zero */
	uint16_t seq;		/* sequence number */
};


/* shared with secrets.c */
extern PSRAM_NOINIT uint8_t io_buf[STORAGE_BLOCK_SIZE];


/*
 * block_read returns the decrypted payload in the buffer at "payload".
 * When calling block_read, payload_len points to the size of the buffer. After
 * a successful read, the length of the decrypted payload is stored in
 * *payload_len.
 *
 * If the block type is anything other than bt_data, neither sequence number
 * nor payload data are returned. If the sequence number is not needed, a NULL
 * pointer can be passed for "seq".
 *
 * If "payload" is NULL, only the block type (without resolving whether what
 * looks like bt_data is really valid) is returned, but no attempt is made to
 * decrypt the block's content.
 */
enum block_type block_read(const struct dbcrypt *c, uint16_t *seq,
    void *payload, unsigned *payload_len, unsigned n);

/*
 * block_write requires the block to be erased. Note that attempting to write
 * to a block that is not completelly erased is likely to produce an invalid
 * block, losing any (valid) data that may have been stored there before.
 */
bool block_write(const struct dbcrypt *c, enum block_type type, uint16_t seq,
    const void *payload, unsigned length, unsigned n);

bool block_delete(unsigned n);

#endif /* !BLOCK_H */
