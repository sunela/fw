/*
 * dbcrypt.h - Database encryption
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef DBCRYPT_H
#define	DBCRYPT_H

#include <stdbool.h>


struct dbcrypt;

#define	DB_NONCE_SIZE	24


/*
 * "Content" is all the encrypted data, including status, payload, hash, and
 * reserved bytes.
 */

bool db_encrypt(const struct dbcrypt *c, void *block, const void *content);
bool db_decrypt(const struct dbcrypt *c, void *content, const void *block);

#endif /* !DBCRYPT_H */
