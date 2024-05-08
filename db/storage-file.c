/*
 * storage.c - Plain storage
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#define _GNU_SOURCE /* for fallocate(2) */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "storage.h"


#define	DEFAULT_FILE_NAME	"_storage"
#define	DEFAULT_FILE_BLOCKS	2048


const char *storage_file = DEFAULT_FILE_NAME;

static int fd = -1;
static unsigned total_blocks;


static void create_storage(void)
{
	fd = open(storage_file, O_RDWR);
	if (fd < 0) {
		fd = open(storage_file, O_CREAT | O_RDWR, 0666);
		if (fd < 0) {
			perror(storage_file);
			exit(1);
		}
		if (fallocate(fd, 0, 0,
		    (off_t) DEFAULT_FILE_BLOCKS * STORAGE_BLOCK_SIZE) < 0) {
			perror(storage_file);
			exit(1);
		}
		total_blocks = DEFAULT_FILE_BLOCKS;
	} else {
		struct stat st;

		if (fstat(fd, &st) < 0) {
			perror(storage_file);
			exit(1);
		}
		total_blocks = st.st_size / STORAGE_BLOCK_SIZE;
	}
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


bool storage_write_block(const void *buf, unsigned n)
{
	ssize_t wrote;

	if (fd == -1)
		create_storage();
	assert(n < total_blocks);
	wrote = pwrite(fd, buf, STORAGE_BLOCK_SIZE,
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


unsigned storage_blocks(void)
{
	if (fd == -1)
		create_storage();
	return total_blocks;
}
