/*
 * db.h - Account database
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * TO DO:
 * - "prev" reference for partial order
 * - add hint of what an entry is (for folders and root keys)
 */

#ifndef DB_H
#define	DB_H

#include <stdbool.h>
#include <stdint.h>


/*
 * https://stackoverflow.com/q/4879286/11496135
 */

enum __attribute__((__packed__)) field_type {
	ft_end		= 0,
	ft_prev		= 1,	// reserved for sorting
	ft_id		= 2,
	ft_user		= 3,
	ft_email	= 4,
	ft_pw		= 5,
	ft_hotp_secret	= 6,
	ft_hotp_counter	= 7,
	ft_totp_secret	= 8,
};

/*
 * Encodings:
 *
 * ID, PREV:
 *   Now: <non-NUL characters>
 *   Later: (<non-NUL characters> NUL)+ <non-NUL characters>
 * User, E-Mail, Password:
 * <non-NUL characters>
 * HOTP-Secret, HOTP-Counter, TOTP-Secret:
 * <bytes>
 */

struct db_field {
	enum field_type type;
	uint8_t		len;
	void		*data;
	struct db_field	*next;
};

struct db;

struct db_entry {
	struct db	*db;
	const char	*name;
	uint16_t	seq;
	unsigned	block;
	struct db_field	*fields;
	struct db_entry	*next;
};

struct db_stats {
	unsigned	total;
	unsigned	erased;
	unsigned	deleted;
	unsigned	empty;
	unsigned	invalid;
	unsigned	error;
	unsigned	data;
};

struct db_span;

struct db {
	const struct dbcrypt *c;
	struct db_stats stats;
	struct db_span *erased;
	struct db_span *deleted;
	struct db_span *empty;
	struct db_entry	*entries;
};


struct db_entry *db_new_entry(struct db *db, const char *name);
bool db_change(struct db_entry *de, enum field_type type,
    const void *data, unsigned size);
bool db_delete(struct db_entry *de);

bool db_iterate(struct db *db, bool (*fn)(void *user, struct db_entry *de),
    void *user);

void db_stats(const struct db *db, struct db_stats *s);

bool db_open(struct db *db, const struct dbcrypt *c);
void db_close(struct db *db);

#endif /* !DB_H */
