/*
 * rmt-db.h - Remote control interface: Database access
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef RMT_DB_H
#define	RMT_DB__H

enum rdb_op {
	RDOP_NULL	= 0,	/* null request, returns an empty reponse */
	RDOP_INVALID	= 1,	/* invalid request, returns an error */
	RDOP_LS		= 2,	/* list all the accounts */
	RDOP_NOT_FOUND	= 3,	/* entry not found, returns an error */
	RDOP_SHOW	= 4,	/* list all fields */
	RDOP_REVEAL	= 5,	/* display field content on device */
	RDOP_BUSY	= 6,	/* previous action is still on-going, returns
				   an error */
};


void rmt_db_poll(void);
void rmt_db_init(void);

#endif /* !RMT_DB_H */
