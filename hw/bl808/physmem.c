/*
 * physmem.c - Physical memory allocator
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * Based on ben-blinkenlights/ubb-vga/physmem.c, which was licensed as follows:
 *
 * Written 2011 by Werner Almesberger
 * Copyright 2011 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "debug.h"
#include "physmem.h"


#define	PAGEMAP_FILE	"/proc/self/pagemap"
#define	PAGE_SIZE	sysconf(_SC_PAGE_SIZE)

#define	PM_PSHIFT	55	/* page size - OBSOLETE*/
#define	PM_PSHIFT_MASK	63
#define	PM_SWAPPED	62
#define	PM_PRESENT	63

#define	ALIGN		32	/* DMA transfer size */


static void align_brk(int alignment)
{
	unsigned long addr, remainder;

	addr = (unsigned long) sbrk(0);
	remainder = addr % alignment;
	if (remainder)
		sbrk(alignment - remainder);
}


void **calloc_phys_vec(size_t n, size_t size)
{
	void **vec;
	unsigned long pos;
	size_t i;

	vec = malloc(sizeof(void *) * n);
	if (!vec) {
		perror("malloc");
		exit(1);
	}

	for (i = 0; i != n; i++) {
		align_brk(ALIGN);
		pos = (unsigned long) sbrk(0);
		if ((pos & ~(PAGE_SIZE - 1)) !=
		    ((pos + size) & ~(PAGE_SIZE - 1)))
			sbrk(PAGE_SIZE - (pos & (PAGE_SIZE - 1)));
		vec[i] = sbrk(size);
		memset(vec[i], 0, size);
		if (mlock((void *) vec[i], size)) {
			perror("mlock");
			exit(1);
		}
	}
	return vec;
}


static unsigned long xlat_one(int fd, unsigned long vaddr)
{
	unsigned long page, offset, paddr;
	ssize_t got;
	uint64_t map;

	offset = vaddr & (PAGE_SIZE - 1);
	page = vaddr / PAGE_SIZE;
	if (lseek(fd, page * 8, SEEK_SET) < 0) {
		perror("lseek");
		exit(1);
	}
	got = read(fd, &map, 8);
	if (got < 0) {
		perror("read");
		exit(1);
	}
	if (got != 8) {
		debug("bad read: got %d instead of 8\n", (int) got);
		exit(1);
	}
	if (!((map >> PM_PRESENT) & 1)) {
		debug("page %lu is not present\n", page);
		abort();
	}
	if ((map >> PM_SWAPPED) & 1) {
		debug("page %lu is swapped\n", page);
		abort();
	}
	paddr = ((map & 0x7fffffffffffffULL) * PAGE_SIZE) | offset;
//	debug("0x%lx -> 0x%lx\n", vaddr, paddr);
	return paddr;
}


unsigned long *xlat_virt(void *const *v, size_t n)
{
	unsigned long *res;
	int fd;
	size_t i;

	res = malloc(n * sizeof(unsigned long));
	if (!res) {
		perror("malloc");
		exit(1);
	}
	fd = open(PAGEMAP_FILE, O_RDONLY);
	if (fd < 0) {
		perror(PAGEMAP_FILE);
		exit(1);
	}
	for (i = 0; i != n; i++)
		res[i] = xlat_one(fd, (unsigned long) v[i]);
	close(fd);
	return res;
}
