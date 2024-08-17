/*
 * rmt-db.h - Remote control interface: Database access
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef RMT_DB_H
#define	RMT_DB__H

enum rdb_op {
	RDOP_NULL,	/* null request, returns an empty reponse */
	RDOP_INVALID,	/* invalid request, returns an error */
	RDOP_LS,	/* list all the accounts */
};


void rmt_db_poll(void);
void rmt_db_init(void);

#endif /* !RMT_DB_H */
