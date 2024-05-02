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
		user:	"user@mail.com",
		pw:	"Geheim",
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
	{
		name:	"TOTP",
		token:	{
			/*
			 * @@@ we store the secret in base32 since all the test
			 * pages use URLs, where the secret is base32-encoded.
			 * for real use, this should be binary.
			 */
			secret_size:	1,	/* @@@ mark as "in use" */
			secret:		(uint8_t *)
					    "GZ4FORKTNBVFGQTFJJGEIRDOKY",
			type:		tt_totp,
			algo:		ta_sha1,
		},
	},
};

struct account *accounts = dummy_accounts;

unsigned n_accounts = sizeof(dummy_accounts) / sizeof(struct account);
