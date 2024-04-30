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


struct mbox {
	bool enabled;
	void *buf;
	size_t size;		/* buffer size */
	uint32_t length;	/* receive only if length > 0 */
		/* Note: read/write of "length" must be atomic. */
};


bool mbox_deposit(volatile struct mbox *mbox, const void *data, size_t length)
{
	if (!mbox->enabled)
		return 0;
	if (mbox->length)
		return 0;
	assert(length);
	assert(length <= mbox->size);
	memcpy(mbox->buf, data, length);
	assert(!mbox->length);
	mbox->length = length;
	return 1;
}


size_t mbox_retrieve(volatile struct mbox *mbox, void *data, size_t size)
{
	size_t len = mbox->length;

	if (!mbox->enabled)
		return 0;
	if (!len)
		return 0;
	if (data) {
		assert(size >= len);
		memcpy(data, mbox->buf, len);
		assert(mbox->length == len);
	}
	mbox->length = 0;
	return 1;
}


void mbox_enable(volatile struct mbox *mbox)
{
	assert(!mbox->enabled);
	mbox->length = 0;
	mbox->enabled = 1;
}


void mbox_disable(volatile struct mbox *mbox)
{
	assert(mbox->enabled);
	mbox->enabled = 0;
	mbox->length = 0;
}


void mbox_init(struct mbox *mbox, void *buf, size_t size)
{
	mbox->buf = buf;
	mbox->size = size;
	mbox->enabled = 0;
	mbox->length = 0;
}
