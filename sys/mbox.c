/*
 * mbox.c - Mailboxes for communication with interrupt handlers
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "mbox.h"


bool mbox_deposit(volatile struct mbox *mbox, const void *data, size_t length)
{
	if (!mbox->enabled)
		return 0;
	if (mbox->length >= 0)
		return 0;
	assert(mbox->buf);
	assert(length <= mbox->size);
	memcpy(mbox->buf, data, length);
	assert(mbox->length < 0);
	mbox->length = length;
	return 1;
}


ssize_t mbox_retrieve(volatile struct mbox *mbox, void *data, size_t size)
{
	ssize_t len = mbox->length;

	if (!mbox->enabled)
		return 0;
	if (len < 0)
		return -1;
	if (data) {
		assert(size >= (size_t) len);
		memcpy(data, mbox->buf, len);
		assert(mbox->length == len);
	}
	mbox->length = -1;
	return len;
}


void mbox_enable(volatile struct mbox *mbox)
{
	assert(!mbox->enabled);
	mbox->length = -1;
	mbox->enabled = 1;
}


void mbox_enable_buf(volatile struct mbox *mbox, void *buf, size_t size)
{
	assert(!mbox->enabled);
	mbox->buf = buf;
	mbox->size = size;
	mbox->length = -1;
	mbox->enabled = 1;
}


void mbox_disable(volatile struct mbox *mbox)
{
	assert(mbox->enabled);
	mbox->enabled = 0;
	mbox->length = -1;
}


void mbox_init(struct mbox *mbox, void *buf, size_t size)
{
	mbox->buf = buf;
	mbox->size = size;
	mbox->enabled = 0;
	mbox->length = -1;
}
