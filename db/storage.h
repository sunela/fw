/*
 * storage.h - Plain storage
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef STORAGE_H
#define	STORAGE_H

#include <stdbool.h>


#define	STORAGE_BLOCK_SIZE	1024


#ifndef SDK

#define	DEFAULT_DB_FILE_NAME	"dummy.db"


extern const char *storage_file;

#endif /* !SDK */


unsigned storage_blocks(void);
unsigned storage_erase_size(void);

bool storage_read_block(void *buf, unsigned n);
bool storage_write_block(const void *buf, unsigned n);
bool storage_erase_blocks(unsigned n, unsigned n_blocks);

#endif /* !STORAGE_H */
