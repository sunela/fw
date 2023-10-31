/*
 * accounts.h - Dummy account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef ACCOUNTS_H
#define	ACCOUNTS_H

struct account {
	const char	*name;
	const char	*user;
	const char	*pw;
};


extern const struct account *accounts;
extern unsigned n_accounts;

#endif /* !ACCOUNTS_H */
