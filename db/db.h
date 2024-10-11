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

#include "hal.h"
#include "storage.h"


#define	MAX_NAME_LEN	16	/* maximum length of an entry name */
#define	MAX_STRING_LEN	64	/* maximum length of user, email, pw */
#define	MAX_SECRET_LEN	20	/* maximum bytes of HOTP/TOTP secret */


/*
 * Use of "packed":
 * https://stackoverflow.com/q/4879286/11496135
 *
 * To add a new field,
 * - add it here, to "enum field_type"
 * - add it to tools/accenc.py:keys, in the same order as in "enum field_type"
 * - add it to db/db.c:order2ft
 * - define now to encode it in tools/accenc.py:encode
 * - define how to dump it in main/script.c:dump_entry
 * - add it to rmt/rmt-db.c:rmt_db_poll, case RDS_RES:RDOP_SHOW
 * - consider adding it to webui/main,js:SUNELA_... and field_name
 * - if the field is shown in the fields list,
 *   - add it to ui/ui_account.c:delete_field
 *   - add it to ui/ui_account.c:ui_account_open
 *   - in to ui/ui_field.c, add it to ui_field_add_open, ui_field_edit_open,
 *     field_edited. and possible ui_field_more
 * - if the field can be revealed,
 *   - add it to rmt/rmt-db.c:rmt_db_poll, case RDS_IDLE:RDOP_REVEAL
 *   - add it to ui/ui_rmt.c:ui_rmt_reveal
 *   - add it to tools/io.c:reveal
 *   - add it to webui/main.js:field_convert.hidden
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
	ft_dir		= 11,
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
	unsigned	block;		/* 0 if entry is virtual */
	bool		defer;		/* defer writing changes to storage */
	struct db_field	*fields;
	struct db_entry	*next;
	struct db_entry	*children;	/* NULL if not a directory */
};

struct db_stats {
	unsigned	total;
	unsigned	erased;
	unsigned	deleted;
	unsigned	empty;
	unsigned	invalid;
	unsigned	error;
	unsigned	data;
	unsigned	special;
};

struct db_span;

struct db {
	const struct dbcrypt *c;
	unsigned generation; /* generation number, to detect changes */
	struct db_stats stats;
	struct db_span *erased;
	struct db_span *deleted;
	struct db_span *empty;
	struct db_entry	*entries;
	struct db_entry *dir;	/* NULL for the root directory */
	int settings_block;
};


extern PSRAM_NOINIT uint8_t payload_buf[STORAGE_BLOCK_SIZE];
	// @@@ beyond-worst-case size

extern struct db main_db;
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
bool db_rename(struct db_entry *de, const char *name);

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

/*
 * db_is_dir and db_is_account return 1 if "de" is a directory or account.
 * They return 0 if it is clearly the other, or if it can still be either
 * account or directory.
 */
bool db_is_dir(const struct db_entry *de);
bool db_is_account(const struct db_entry *de);

/*
 * db_mkdir turns an empty entry into a directory. db_mkentry turns an empty
 * directory into an empty entry.
 */
void db_mkdir(struct db_entry *de);
void db_mkentry(struct db_entry *de);

void db_chdir(struct db *db, struct db_entry *de);
struct db_entry *db_dir_parent(const struct db *db);

/*
 * db_pwd returns NULL if we are at the top-level directory, or the name of the
 * of the directory entry. It does NOT return the whole path.
 */
const char *db_pwd(const struct db *db);

bool db_update_settings(struct db *db, uint16_t seq,
    const void *payload, unsigned length);

void db_stats(const struct db *db, struct db_stats *s);

bool db_open_progress(struct db *db, const struct dbcrypt *c,
    void (*progress)(void *user, unsigned i, unsigned n), void *user);
bool db_open(struct db *db, const struct dbcrypt *c);
void db_close(struct db *db);

bool db_is_erased(void);

void db_init(void);

#endif /* !DB_H */
