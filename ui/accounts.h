/*
 * accounts.h - Dummy account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef ACCOUNTS_H
#define	ACCOUNTS_H

#include <stdint.h>


struct account {
	const char	*name;	/* required */
	const char	*user;	/* may be NULL */
	const char	*pw;	/* may be NULL */
	struct token {
		unsigned	secret_size; /* if zero, ignore this struct */
		uint8_t		*secret;
		enum token_type {
			tt_hotp,
		} type;
		enum token_algo {
			ta_sha1,
		} algo;
		uint64_t	counter; /* for HOTP */
	} token;
};


extern const struct account *accounts;
extern unsigned n_accounts;

#endif /* !ACCOUNTS_H */
