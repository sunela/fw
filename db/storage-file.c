/*
 * storage-file.c - Plain storage
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * We emulate the behaviour of Flash memory:
 * - Blocks can only erased in groups of ERASE_SIZE blocks, which is more than
 *   one block.
 * - Only erasing writes "one" bits.
 * - Writing (without erasing) only writes zero bits.
 */

#define _GNU_SOURCE /* for fallocate(2) */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "storage.h"


#define	DEFAULT_FILE_NAME	"_storage"
#define	DEFAULT_FILE_BLOCKS	2048
#define	ERASE_SIZE		4	/* erasing erases four blocks */


const char *storage_file = DEFAULT_FILE_NAME;

static int fd = -1;
static unsigned total_blocks;
static uint32_t tmp[STORAGE_BLOCK_SIZE / 4];


static void create_storage(void)
{
	fd = open(storage_file, O_RDWR);
	if (fd < 0) {
		fd = open(storage_file, O_CREAT | O_RDWR, 0666);
		if (fd < 0) {
			perror(storage_file);
			exit(1);
		}
		total_blocks = DEFAULT_FILE_BLOCKS;
		if (!storage_erase_blocks(0, DEFAULT_FILE_BLOCKS))
			exit(1);
	} else {
		struct stat st;

		if (fstat(fd, &st) < 0) {
			perror(storage_file);
			exit(1);
		}
		total_blocks = st.st_size / STORAGE_BLOCK_SIZE;
	}
}


unsigned storage_blocks(void)
{
	if (fd == -1)
		create_storage();
	return total_blocks;
}


unsigned storage_erase_size(void)
{
	return ERASE_SIZE;
}


bool storage_read_block(void *buf, unsigned n)
{
	ssize_t got;

	if (fd == -1)
		create_storage();
	assert(n < total_blocks);
	got = pread(fd, buf, STORAGE_BLOCK_SIZE,
	    (off_t) n * STORAGE_BLOCK_SIZE);
	if (got < 0) {
		perror(storage_file);
		exit(1);
	}
	if (got != STORAGE_BLOCK_SIZE) {
		fprintf(stderr, "%s: short write\n", storage_file);
		exit(1);
	}
	return 1;
}


static bool do_write_block(const void *buf, unsigned n)
{
	ssize_t wrote;

	if (fd == -1)
		create_storage();
	assert(n < total_blocks);
	wrote = pwrite(fd, tmp, STORAGE_BLOCK_SIZE,
	    (off_t) n * STORAGE_BLOCK_SIZE);
	if (wrote < 0) {
		perror(storage_file);
		exit(1);
	}
	if (wrote != STORAGE_BLOCK_SIZE) {
		fprintf(stderr, "%s: short write\n", storage_file);
		exit(1);
	}
	if (fdatasync(fd) < 0) {
		perror(storage_file);
		exit(1);
	}
	return 1;
}


bool storage_write_block(const void *buf, unsigned n)
{
	ssize_t got;
	uint32_t *p;
	const uint32_t *q = buf;
	bool ret;

	if (fd == -1)
		create_storage();
	assert(n < total_blocks);

	/* read the old block, so that we can preserve "1" bits */
	got = pread(fd, tmp, STORAGE_BLOCK_SIZE,
	    (off_t) n * STORAGE_BLOCK_SIZE);
	if (got < 0) {
		perror(storage_file);
		exit(1);
	}
	if (got != STORAGE_BLOCK_SIZE) {
		fprintf(stderr, "%s: short write\n", storage_file);
		exit(1);
	}

	/* writing can only turn "1"" into "0" */
	for (p = tmp; p != (void *) tmp + sizeof(tmp); p++)
		*p &= *q++;

	ret = do_write_block(tmp, n);

	memset(tmp, 0, sizeof(tmp));
	return ret;
}


bool storage_erase_blocks(unsigned n, unsigned n_blocks)
{
	assert(!(n % ERASE_SIZE));
	assert(!(n_blocks % ERASE_SIZE));
	memset(tmp, 0xff, STORAGE_BLOCK_SIZE);
	while (n_blocks--)
		if (!do_write_block(tmp, n++))
			return 0;
	return 1;
}
