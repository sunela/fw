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
 * 
 * "Content" is all the encrypted data, including status, payload, hash, and
 * reserved bytes.
 *
 * db_encrypt returns 1 if the encryption was successful, 0 otherwise.
 * db_decrypt returns -1 if decrypting failed, the length of the decrypted
 * content otherwise.
 */

bool db_encrypt(const struct dbcrypt *c, void *block, const void *content,
    unsigned length);
int db_decrypt(const struct dbcrypt *c, void *content, unsigned size,
    const void *block);

struct dbcrypt *dbcrypt_init(const void *sk, unsigned size);
void dbcrypt_add_reader(struct dbcrypt *c, const void *pk, unsigned size);
void dbcrypt_free(struct dbcrypt *c);

#endif /* !DBCRYPT_H */
