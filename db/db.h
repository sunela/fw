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


#define	MAX_NAME_LEN	16	/* maximum length of an entry name */
#define	MAX_STRING_LEN	64	/* maximum length of user, email, pw */
#define	MAX_SECRET_LEN	20	/* maximum bytes of HOTP/TOTP secret */


/*
 * Use of "packed":
 * https://stackoverflow.com/q/4879286/11496135
 */

enum __attribute__((__packed__)) field_type {
	ft_end		= 0,
	ft_id		= 1,
	ft_prev		= 2,	// place after this entry 
	ft_user		= 3,
	ft_email	= 4,
	ft_pw		= 5,
	ft_hotp_secret	= 6,
	ft_hotp_counter	= 7,
	ft_totp_secret	= 8,
	ft_comment	= 9,
	ft_pw2		= 10,
};

/*
 * Encodings:
 *
 * ID, prev:
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
	char		*name;
	uint16_t	seq;
	unsigned	block;
	bool		defer;	/* defer writing changes to storage */
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


extern const enum field_type order2ft[];
extern uint8_t ft2order[];
extern const unsigned field_types;


/* Functions only exported only for testing of the topological sort. */

unsigned db_tsort(struct db *db);
struct db_entry *db_dummy_entry(struct db *db, const char *name,
    const char *prev);
void db_open_empty(struct db *db, const struct dbcrypt *c);

struct db_field *db_field_find(const struct db_entry *de, enum field_type type);
bool db_change_field(struct db_entry *de, enum field_type type,
    const void *data, unsigned size);
bool db_delete_field(struct db_entry *de, struct db_field *f);

/*
 * db_entry_defer_update(..., 1) disables writing changes to the entry back to
 * storage. db_entry_defer_update(..., 0) rewrites the entry. (We currently
 * assume that is has been changed, so we always rewrite it. This may change in
 * the future.)
 *
 * db_entry_defer_update(..., 1) can be used multiple times on an entry before
 * db_entry_defer_update(..., 0). Also, any number of entries may defer updates
 * concurrently.
 *
 * db_entry_defer_update returns 0 if an update was attempted but failed. It
 * returns 1 in all other cases.
 */

bool db_entry_defer_update(struct db_entry *de, bool defer);

struct db_entry *db_new_entry(struct db *db, const char *name);

/*
 * Adjusts "prev" fields in the database such that entry "e" is sorted after
 * entry "after":
 * - the "prev" field of "e" is set up point to "after"
 * - all "prev" fields pointing to "e" are changed to the value of "e"'s "prev"
 *   field
 * - all "prev" fields pointing to "after" are changed to point to "e"
 * If "after" is NULL, the entry is inserted at the beginning of the database.
 */

void db_move_after(struct db_entry *e, const struct db_entry *after);

/*
 * Adjusts "prev" fields in the database such that entry "e" is sorted before
 * entry "after", or at the end of the list if "after" is NULL.
 *
 * See "move_after" for details.
 */

void db_move_before(struct db_entry *e, const struct db_entry *before);

bool db_delete_entry(struct db_entry *de);
bool db_iterate(struct db *db, bool (*fn)(void *user, struct db_entry *de),
    void *user);

void db_stats(const struct db *db, struct db_stats *s);

bool db_open_progress(struct db *db, const struct dbcrypt *c,
    void (*progress)(void *user, unsigned i, unsigned n), void *user);
bool db_open(struct db *db, const struct dbcrypt *c);
void db_close(struct db *db);

void db_init(void);

#endif /* !DB_H */
