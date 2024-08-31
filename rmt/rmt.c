/*
 * rmt.c - Remote control interface
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/types.h>

#include "hal.h"
#include "debug.h"
#include "mbox.h"
#include "rmt.h"


#if 0
#define	DEBUG(...) debug(__VA_ARGS__)
#else
#define	DEBUG(...) do {} while (0)
#endif


enum rmt_state {
	RS_IDLE,	/* no request */
	RS_REQ,		/* receiving request blocks */
	RS_RES,		/* sending response blocks */
};

struct rmt {
	enum rmt_state state;
	bool (*reset)(void);	/* called on protocol reset (from interrupt
				   handler !) if not NULL */
	struct mbox in;
	struct mbox out;
	uint8_t mbox_buf[RMT_MAX_LEN];
	uint8_t usb_buf[RMT_MAX_LEN];
};


/*
 * @@@ Do NOT use PSRAM here !
 *
 * We need rmt_use.state to be RS_IDLE (zero) and the mailboxes in rmt_usb to
 * be initialized (disabled) when the system starts.
 *
 * With PSRAM, no such initialization would happen: First, PSRAM uses the
 * psram_noinit segment, which is not initialized at all. Second, even if we
 * used psram_data, which sounds as if it would be initialized, this still
 * wouldn't work, since the code for it in
 * bouffalo_sdk/bsp/board/bl808dk/board.c:board_init is
 * commented out.
 */

struct rmt rmt_usb;


/*
 * We map the remote control interface to USB as follows:
 * request:
 *   TO_DEV | SUNELA_RMT, non-zero length
 *   ...
 *   TO_DEV | SUNELA_RMT, zero length
 * optional response:
 *   FROM_DEV | SUNELA_RMT, non-zero length
 *   ...
 * FROM_DEV | SUNELA_RMT, zero length
 *
 * @@@ Note: control transfers are not a great choice here, because we can't
 * wait for a response, but have to fail a request if no response is ready, and
 * the host has to poll.
 */


/* --- Remote interface API ------------------------------------------------ */


int rmt_request(struct rmt *rmt, void *buf, unsigned size)
{
	ssize_t got;

	assert(rmt->state == RS_IDLE || rmt->state == RS_REQ);
	got = mbox_retrieve(&rmt->in, buf, size);
	if (got >= 0)
		DEBUG("rmt_request(%u): got %d\n", rmt->state, (int) got);
	if (got < 0)
		return rmt->state == RS_IDLE ? -1 : 0;
	if (got > 0) {
		rmt->state = RS_REQ;
		return got;
	}
	rmt->state = RS_RES;
//	mbox_disable(&rmt->in);
//	mbox_enable(&rmt->out);
	return -1;
}


bool rmt_responsev(struct rmt *rmt, ...)
{
	va_list ap;
	bool ok;

	assert(rmt->state == RS_RES);
	va_start(ap, rmt);
	ok = mbox_vdepositv(&rmt->out, ap);
	DEBUG("rmt_responsev(%u): success %u\n", rmt->state, ok);
	va_end(ap);
	return ok;
}


bool rmt_response(struct rmt *rmt, const void *buf, unsigned len)
{
	DEBUG("rmt_response(%u/%u) len %u\n", rmt->state, RS_RES, len);
	return rmt_responsev(rmt, buf, len, NULL);
}


bool rmt_end(struct rmt *rmt)
{
	bool ok;

	assert(rmt->state == RS_RES);
	ok = mbox_deposit(&rmt->out, NULL, 0);
	DEBUG("rmt_end(%u): success %u\n", rmt->state, ok);
	if (!ok)
		return 0;
	rmt->state = RS_IDLE;
	// @@@ this is very close to a race condition: we use the same buffer
	// for in and out, but have separate mailbox states.
	return 1;
}


bool rmt_poll(struct rmt *rmt)
{
	return 1;
}


/* --- Interface to the USB stack ------------------------------------------ */


bool rmt_query(struct rmt *rmt, uint8_t **data, uint32_t *len)
{
	ssize_t got;

	got = mbox_retrieve(&rmt->out, rmt->usb_buf, RMT_MAX_LEN);
	if (got < 0)
		return 0;
	*data = rmt->usb_buf;
	*len = got;
	return 1;
}


bool rmt_arrival(struct rmt *rmt, const void *data, uint32_t len)
{
	bool ok;

	/*
	 * @@@ this protocol reset logic lets us proceed if we've received a
	 * complete header, and then the host abandoned the transfer and tried
	 * sending a new request.
	 * However, we don't synchronize if the host did not finish sending the
	 * header. (Eventually, the protocol on top of rmt should figure out
	 * that something is amiss, and the states should converge. FSS.)
	 */
	if (rmt->state == RS_RES && rmt->reset && rmt->reset())
		rmt->state = RS_IDLE;
	assert(rmt->state == RS_IDLE || rmt->state == RS_REQ);
	ok = mbox_deposit(&rmt->in, data, len);
	DEBUG("rmt_arrival: success %u\n", ok);
	return ok;
}


/* --- Interface to the USB stack ------------------------------------------ */


void rmt_open(struct rmt *rmt)
{
	rmt->state = RS_IDLE;
	rmt->reset = NULL;
	mbox_init(&rmt->in, rmt->mbox_buf, RMT_MAX_LEN);
	mbox_init(&rmt->out, rmt->mbox_buf, RMT_MAX_LEN);
	mbox_enable(&rmt->in);
	// @@@ we shouldn't enable both mailboxes at the same time
	mbox_enable(&rmt->out);
}


void rmt_set_reset(struct rmt *rmt, bool (*reset)(void))
{
	rmt->reset = reset;
}


void rmt_close(struct rmt *rmt)
{
	switch (rmt->state) {
	case RS_IDLE:
	case RS_REQ:
		mbox_disable(&rmt->in);
		break;
	case RS_RES:
		mbox_disable(&rmt->out);
		break;
	}
	rmt->state = RS_IDLE;
}
