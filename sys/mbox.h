/*
 * mbox.c - Mailboxes for communication with interrupt handlers
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef MBOX_H
#define	MBOX_H

/*
 * Mailboxes are intended for communication between the single-threaded
 * application and interrupt handlers. They do NOT work for:
 * - multiple interrupt handlers accessing the same mailbox, if these
 *   interrupts can preempt each other,
 * - multithreading,
 * - multiple readers or multiple writers (e.g., if an interrupt handler can
 *   deposit and then retrieve/discard a message).
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>


struct mbox {
	bool enabled;
	void *buf;
	size_t size;		/* buffer size */
	int32_t length;		/* receive only if length >= 0 */
		/* Note: read/write of "length" must be atomic. */
};


#define	MBOX_INIT	{ .enabled = 0, .buf = NULL, .size = 0, .length = -1 }
#define	MBOX_INIT_BUF(buf, size) \
	{ .enabled = 0, .buf = (buf), .size = (size), .length = 0 }


/*
 * If no operation was performed, mbox_deposit returns 0, and mbox_retrieve
 * returns a negative number. None of the functions block or loop. Messages
 * must not exceed the maximum buffer size. They may be shorter, and can be
 * empty.
 *
 * The buffer given to mbox_retrieve must be at least large enough to store the
 * message. There is no partial retrieval. To avoid spurious errors, it is
 * recommended to make it as large as the message size of the mailbox.
 *
 * mbox_retrieve can be called with data = NULL, to discard any message that
 * may be in the mailbox. There is no way to probe for the presence of a
 * message without retrieving it. Note that mbox_enable and mbox_disable also
 * discard any pending messages.
 *
 * mbox_depositv is like mbox_deposit, but its arguments are a list of
 * (pointer, length) pairs. The last pointer must be NULL, and does not have to
 * be followed by a length. mbox_vdepositv is like mbox_depositv, but operates
 * on the argument pointer, similar to vprintf and printf.
 */

bool mbox_vdepositv(volatile struct mbox *mbox, va_list ap);
bool mbox_depositv(volatile struct mbox *mbox, ...);
bool mbox_deposit(volatile struct mbox *mbox, const void *data, size_t length);
ssize_t mbox_retrieve(volatile struct mbox *mbox, void *data, size_t size);

void mbox_enable(volatile struct mbox *mbox);
void mbox_enable_buf(volatile struct mbox *mbox, void *buf, size_t size);
void mbox_disable(volatile struct mbox *mbox);

/*
 * After initialization, mailboxes are disabled. If the buffer is to be set
 * with mbox_enable_buf, it can be initialized to NULL with size 0.
 */

void mbox_init(struct mbox *mbox, void *buf, size_t size);

#endif /* !MBOX_H */
