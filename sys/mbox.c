/*
 * mbox.c - Mailboxes for communication with interrupt handlers
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "mbox.h"


bool mbox_vdepositv(volatile struct mbox *mbox, va_list ap)
{
	va_list aq;
	size_t total = 0;
	void *p;

	va_copy(aq, ap);
	if (!mbox->enabled)
		return 0;
	if (mbox->length >= 0)
		return 0;
	assert(mbox->buf);

	while (va_arg(ap, const void *))
		total += va_arg(ap, size_t);

	assert(total <= mbox->size);

	p = mbox->buf;
	while (1) {
		const void *data;
		size_t length;

		data = va_arg(aq, const void *);
		if (!data)
			break;
		length = va_arg(aq, size_t);
		memcpy(p, data, length);
		p += length;
	}

	assert(mbox->length < 0);
	mbox->length = total;

	return 1;
}


bool mbox_depositv(volatile struct mbox *mbox, ...)
{
	va_list ap;
	bool ret;

	va_start(ap, mbox);
	ret = mbox_vdepositv(mbox, ap);
	va_end(ap);
	return ret;
}


bool mbox_deposit(volatile struct mbox *mbox, const void *data, size_t length)
{
	return mbox_depositv(mbox, data, length, NULL);
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
