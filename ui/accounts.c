/*
 * accounts.c - Dummy account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "accounts.h"


static const struct account dummy_accounts[] = {
	{ "demo", 	"user@mail.com", 	"Geheim" },
	{ "2nd",	"account name",		"password" },
	{ "more",	"something",		"else" },
};

const struct account *accounts = dummy_accounts;

unsigned n_accounts = sizeof(dummy_accounts) / sizeof(struct account);
