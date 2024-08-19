/*
 * rmt.h - Remote control interface
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef RMT_H
#define	RMT_H

#include <stdbool.h>


#define	RMT_MAX_LEN	256


struct rmt;


extern struct rmt rmt_usb;


/*
 * Each transaction consists of a request and a response.
 *
 * Requests consist of one or more request blocks. Each request is answered
 * with a response of zero or more response blocks. A new request can only be
 * sent after the response to the previous request has been received. Request
 * and response blocks must have a length from 1 to RMT_MAX_LEN bytes.
 */


/* --- Remote interface API ------------------------------------------------ */


/*
 * If a request block is pending, rmt_request returns the block length. If no
 * block is pending, but more blocks of the same request may arrive, it returns
 * zero. If no request is pending or being received, e.g., before the first
 * request or after a transaction has entered the respose phase, rmt_request
 * returns a negative number. All request block must be received before calling
 * rmt_response or rmt_end.
 */

int rmt_request(struct rmt *rmt, void *buf, unsigned size);

/*
 * rmt_response enqueues a response block. The block may be sent immediately,
 * or require additional asyncronous operations. If rmt_response returns zero,
 * the response could not be accepted (e.g., because a previous response is
 * being sent) and the operation has to be retried.
 *
 * rmt_responsev is like rmt_response, but its arguments are a list of
 * (pointer, length) pairs. The last pointer must be NULL, and does not have to
 * be followed by a length.
 */

bool rmt_responsev(struct rmt *rmt, ...);
bool rmt_response(struct rmt *rmt, const void *buf, unsigned len);

/*
 * rmt_end ends a response, and must always be called after processing a
 * request, even if there are no response blocks. Before the next call to
 * rmt_request, the caller must call rmt_end until it returns 1.
 */

bool rmt_end(struct rmt *rmt);

/*
 * Perform asynchronous communication actions. Return 0 if the endpoint is
 * busy, and no calls to other rmt_* functions (except rmt_close) are allowed.
 */

bool rmt_poll(struct rmt *rmt);

/*
 * Set up the local side of a remote control connection. Establishing the
 * connection require may additional steps, which are performed asynchronously.
 */


/* --- Interface to the USB stack ------------------------------------------ */


/* rmt_query and rmt_arrival are run from interrupt handlers of the SDK ! */

bool rmt_query(struct rmt *rmt, uint8_t **data, uint32_t *len);
bool rmt_arrival(struct rmt *rmt, const void *data, uint32_t len);


/* --- Open/close ---------------------------------------------------------- */


void rmt_open(struct rmt *rmt);

/*
 * Close a remote control connection. If the endpoint is busy sending data at
 * the time rmt_close is called, the data transmission is aborted.
 */

void rmt_close(struct rmt *rmt);

#endif /* !RMT_H */
