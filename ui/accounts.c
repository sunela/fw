/*
 * accounts.c - Dummy account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "accounts.h"


static struct account dummy_accounts[] = {
	{
		name:	"demo",
		user: 	"user@mail.com",
		pw: 	"Geheim",
		token:	{ 0 }
	},
	{
		name:	"2nd",
		user:	"account name",
		pw:	"password",
		token:	{ 0 }
	},
	{
		name:	"more",
		user:	"something",
		pw:	"else",
		token:	{ 0 }
	},
	{
		name:	"HOTP",
		token:	{
			secret_size:	20,
			secret:		(uint8_t *) "12345678901234567890",
			type:		tt_hotp,
			algo:		ta_sha1,
			counter:	0,
		},
	},
};

struct account *accounts = dummy_accounts;

unsigned n_accounts = sizeof(dummy_accounts) / sizeof(struct account);
