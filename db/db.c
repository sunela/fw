/*
 * db.c - Account database
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#define	_GNU_SOURCE	/* for memrchr */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "util.h"
#include "alloc.h"
#include "rnd.h"
#include "span.h"
#include "storage.h"
#include "block.h"
#include "settings.h"
#include "db.h"


static bool update_entry(struct db_entry *de, unsigned new);


PSRAM_NOINIT uint8_t payload_buf[STORAGE_BLOCK_SIZE];
	// @@@ beyond-worst-case size

struct db main_db;
const enum field_type order2ft[] = {
    ft_end, ft_id, ft_prev, ft_dir, ft_user, ft_email, ft_pw, ft_pw2,
    ft_hotp_secret, ft_hotp_counter, ft_totp_secret, ft_comment };
uint8_t ft2order[ARRAY_ENTRIES(order2ft)];
const unsigned field_types = sizeof(ft2order);


/* --- Get an erased block, erasing if needed ------------------------------ */


static int get_erased_block(struct db *db)
{
	unsigned erase_size = storage_erase_size();
	int n;

	while (1) {
		n = span_pull_one(&db->erased);
		if (n >= 0) {
			db->stats.erased--;
			break;
		}
		n = span_pull_erase_block(&db->empty, erase_size);
		if (n >= 0) {
			db->stats.empty -= erase_size;
			if (storage_erase_blocks(n, erase_size)) {
				db->stats.erased += erase_size;
				continue;
			}
			db->stats.error += erase_size;
		}
		n = span_pull_erase_block(&db->deleted, erase_size);
		if (n >= 0) {
			db->stats.deleted -= erase_size;
			if (storage_erase_blocks(n, erase_size)) {
				db->stats.erased += erase_size;
				continue;
			}
			db->stats.error += erase_size;
		}
		return -1;
	}
	return n;
}


/* --- Helper functions ---------------------------------------------------- */


static void free_field(struct db_field *f)
{
	free(f->data);
	free(f);
}


static void free_fields(struct db_entry *de)
{
	while (de->fields) {
		struct db_field *this = de->fields;

		de->fields = this->next;
		free_field(this);
	}
}


static void free_entry(struct db_entry *de)
{
	free(de->name);
	free_fields(de);
	free(de);
}


/* --- Fields of database entries ------------------------------------------ */


static void add_field(struct db_entry *de, enum field_type type,
    const void *data, unsigned len)
{
	struct db_field **anchor;
	struct db_field *f;

	for (anchor = &de->fields; *anchor; anchor = &(*anchor)->next)
		if ((*anchor)->type > type)
			break;
	f = alloc_type(struct db_field);
	f->type = type;
	f->len = len;
	f->data = len ? alloc_size(len) : NULL;
	memcpy(f->data, data, len);
	f->next = *anchor;
	*anchor = f;
	de->db->generation++;
}


bool db_change_field(struct db_entry *de, enum field_type type,
    const void *data, unsigned size)
{
	struct db *db = de->db;
	struct db_field **anchor;
	struct db_field *f;
	int new = -1;

	if (debugging)
		printf("db_change_field: %s.%u -> \"%.*s\"\n",
		    de->name, type, size, (char *) data);
	if (!de->defer) {
		new = get_erased_block(db);
		if (new < 0)
			return 0;
	}

	for (anchor = &de->fields; *anchor; anchor = &(*anchor)->next)
		if ((*anchor)->type >= type)
			break;
	if (*anchor && (*anchor)->type == type) {
		f = *anchor;
		free(f->data);
	} else {
		f = alloc_type(struct db_field);
		f->type = type;
		f->next = *anchor;
		*anchor = f;
	}
	f->len = size;
	f->data = alloc_size(size);
	memcpy(f->data, data, size);

	switch (type) {
	case ft_id:
		free(de->name);

		const void *z = memrchr(data, 0, size);

		if (z) {
			unsigned name_len = size - (z + 1 - data);

			de->name = alloc_size(name_len + 1);
			memcpy(de->name, z + 1, name_len);
			de->name[name_len] = 0;
		} else {
			de->name = alloc_size(size + 1);
			memcpy(de->name, data, size);
			de->name[size] = 0;
		}
		db_tsort(db);
		break;
	case ft_prev:
		db_tsort(db);
		break;
	default:
		db->generation++;
		break;
	}

	return de->defer || update_entry(de, new);
}


bool db_delete_field(struct db_entry *de, struct db_field *f)
{
	struct db *db = de->db;
	struct db_field **anchor;
	int new = -1;

	if (!de->defer) {
		new = get_erased_block(db);
		if (new < 0)
			return 0;
	}

	for (anchor = &de->fields; *anchor != f; anchor = &(*anchor)->next);
	*anchor = f->next;
	free_field(f);

	/* just in case we deleted ft_prev */
	if (!db_tsort(db))
		db->generation++;

	return de->defer || update_entry(de, new);
}


static bool patch_one(struct db_entry *de, const char *new_prefix,
    unsigned new_prefix_len, unsigned old_prefix_len)
{
	const struct db_field *id = de->fields;
	bool ok;

	assert(id->type == ft_id);
	assert(id->len >= old_prefix_len);
	if (id->len == old_prefix_len) {
		ok = db_change_field(de, ft_id, new_prefix, new_prefix_len);
	} else {
		unsigned new_len =
		    id->len - old_prefix_len + new_prefix_len;
		char *tmp;

		assert(!((const uint8_t *) id->data)[old_prefix_len]);
		tmp = alloc_size(new_len);
		memcpy(tmp, new_prefix, new_prefix_len);
		memcpy(tmp + new_prefix_len, id->data + old_prefix_len,
		    id->len - old_prefix_len);
		ok = db_change_field(de, ft_id, tmp, new_len);
		free(tmp);
	}
	return ok;
}


static bool patch_recursive(struct db_entry *de, const char *new_prefix,
    unsigned new_prefix_len, unsigned old_prefix_len)
{
	struct db_entry *e;

#if 0
	const struct db_field *id = de->fields;

	debug("patch_recursive %s: %.*s -> %.*s,  %u -> %u\n",
	    de->name, id->len, (const char *) id->data, new_prefix_len,
	    new_prefix, old_prefix_len, new_prefix_len);
#endif
	if (!patch_one(de, new_prefix, new_prefix_len, old_prefix_len))
		return 0;
	for (e = de->children; e; e = e->next)
		if (!patch_recursive(e, new_prefix, new_prefix_len,
		    old_prefix_len))
			return 0;
	return 1;
}


bool db_rename(struct db_entry *de, const char *name)
{
	const struct db_field *id = de->fields;
	unsigned name_len = strlen(name);
	const void *z;
	bool ok;

	assert(id->type == ft_id);
	z = memrchr(id->data, 0, id->len);
	if (!z)
		return patch_recursive(de, name, name_len, id->len);

	unsigned pos = z - id->data + 1;
	unsigned new_len = pos + name_len;
	char *tmp = alloc_size(new_len);

	assert(pos > 1);
	assert(id->len > pos);
	memcpy(tmp, id->data, pos);
	memcpy(tmp + pos, name, new_len);
	ok = patch_recursive(de, tmp, new_len, id->len);
	free(tmp);
	return ok;
}


struct db_field *db_field_find(const struct db_entry *de, enum field_type type)
{
	struct db_field *f;

	for (f = de->fields; f; f = f->next)
		if (f->type == type)
			return f;
	return NULL;
}


/* --- Database entries: storage operations -------------------------------- */


static bool write_entry(const struct db_entry *de)
{
	const void *end = payload_buf + sizeof(payload_buf);
	uint8_t *p = payload_buf;
	const struct db_field *f;
	bool ok;

	assert(de->block);
	memset(payload_buf, 0, sizeof(payload_buf));
	for (f = de->fields; f; f = f->next) {
		if ((const void *) p + f->len + 2 > end) {
			debug("write_entry: %p + %u + 2 > %p\n",
			    p, f->len, end);
			return 0;
		}
		*p++ = f->type;
		*p++ = f->len;
		memcpy(p, f->data, f->len);
		p += f->len;
	}
	ok = block_write(de->db->c, bt_data, de->seq, payload_buf,
	    p - payload_buf, de->block);
	if (ok)
		de->db->stats.data++;
	return ok;
}


static bool update_entry(struct db_entry *de, unsigned new)
{
	struct db *db = de->db;
	unsigned old = de->block;

	old = de->block;
	de->block = new;
	de->seq++;

	if (!write_entry(de)) {
		// @@@ complain more if block_delete fails ?
		if (block_delete(new)) {
			span_add(&db->deleted, new, 1);
			db->stats.deleted++;
		}
		return 0;
	}
	// @@@ complain if block_delete fails ?
	if (old && block_delete(old)) {
		span_add(&db->deleted, old, 1);
		db->stats.data--;
		db->stats.deleted++;
	}
	return 1;
}


bool db_entry_defer_update(struct db_entry *de, bool defer)
{
	if (!defer) {
		int new;

		new = get_erased_block(de->db);
		if (new < 0)
			return 0;
		if (!update_entry(de, new))
			return 0;
	}
	de->defer = defer;
	return 1;
}


/* --- Database entries: creation  ----------------------------------------- */

/*
 * Sorting: "prev" establishes a partial order [1]. The idea is that entries
 * are sorted alphabetically, but that this sort order can be overridden by
 * setting "prev".
 *
 * The algorithm below does this for any new entry that is added, where the new
 * entry may have a pointer (prev) to the previous entry. However, this is only
 * valid under the following conditions:
 * - the entry referenced by "prev" already exists when the new entry is added,
 *   and
 * - no existing entries reference the new entry as their "prev" entry.
 *
 * If these conditions are not met, we could aim for an alphabetic order, and
 * then perform a stable [2] topological sort [1] on the resulting list.
 * "Stable" means that entries on which the partial order imposes no ordering,
 * either directly or via other entries, are left in their original order with
 * respect to each other.
 *
 * Some practical considerations:
 * https://stackoverflow.com/q/11230881/11496135
 *
 * Note: the conditions of this algorithm are met if adding a new entry for
 * which no "prev" references exist in the database. However, already deleting
 * an old entry and creating a new one with the same name would violate this
 * condition. (I.e., when deleting entries, we don't try to maintain
 * referential integrity [3] by also removing all "prev" entries pointing to
 * it, which can cause the corresponding ordering relations to "come back"
 * later.)
 *
 * Also, given that entries need to be copied when making changes, including
 * automatic ones, like incrementing the HOTP counter, and thus generally move
 * around, we can easily end up with a database where entries are stored in a
 * sequence where the current algorithm fails to produce the expected results.
 *
 * [1] https://en.wikipedia.org/wiki/Topological_sorting#Relation_to_partial_orders
 * [2] https://en.wikipedia.org/wiki/Sorting_algorithm#Stability
 * [3] https://en.wikipedia.org/wiki/Referential_integrity
 */

/*
 * Update:
 * new_entry only sorts alphabetically. We now have db_tsort for proper
 * topological sorting.
 */

static struct db_entry *new_entry(struct db *db, struct db_entry *dir,
    const char *name)
{
	struct db_entry *de, **anchor;

	de = alloc_type(struct db_entry);
	memset(de, 0, sizeof(*de));
	de->db = db;
	for (anchor = dir ? &dir->children : &db->entries; *anchor;
	    anchor = &(*anchor)->next)
		if (strcmp((*anchor)->name, name) > 0)
			break;
	de->next = *anchor;
	*anchor = de;
	return de;
}


struct db_entry *db_new_entry(struct db *db, const char *name)
{
	struct db_entry *de;
	int new;

	new = get_erased_block(db);
	if (new < 0)
		return NULL;
	db->generation++;
	de = new_entry(db, db->dir, name);
	de->name = stralloc(name);
	de->block = new;
	rnd_bytes(&de->seq, sizeof(de->seq));

	unsigned name_len = strlen(name);
	const struct db_entry *parent = db->dir;

	if (parent) {
		struct db_field *id = parent->fields;
		unsigned path_len = id->len + 1 + name_len;
		char *tmp = alloc_size(path_len);

		assert(id->type == ft_id);
		memcpy(tmp, id->data, id->len);
		tmp[id->len] = 0;
		memcpy(tmp + id->len + 1, name, name_len);
		add_field(de, ft_id, tmp, path_len);
		free(tmp);
	} else {
		add_field(de, ft_id, name, name_len);
	}

	db_tsort(db);
	if (write_entry(de))
		return de;
	// @@@ complain
	if (block_delete(new)) {
		span_add(&db->deleted, new, 1);
		db->stats.deleted++;
	}
	return NULL;
}


struct db_entry *db_dummy_entry(struct db *db, const char *name,
    const char *prev)
{
	struct db_entry *de;

	db->generation++;
	de = new_entry(db, db->dir, name);
	de->name = stralloc(name);
	de->block = -1;
	de->defer = 1;
	if (prev)
		add_field(de, ft_prev, prev, strlen(prev));
	return de;
}


/* --- Database entries: sorting ------------------------------------------- */


unsigned db_tsort(struct db *db)
{
	struct tmp {
		struct db_entry *in;
		const struct db_entry *prev;
		struct db_entry *out;
	} *tmp, *t, *t2;
	struct db_entry *e, *e2;
	unsigned n = 0;
	unsigned i;

	for (e = db->entries; e; e = e->next)
		n++;
	if (!n)
		return 0;

	db->generation++;

	/* allocate temporary variables */
	tmp = t = alloc_type_n(struct tmp, n);
	for (e = db->entries; e; e = e->next) {
		struct db_field *f;

		t->in = e;
		t->prev = NULL;
		for (f = e->fields; f; f = f->next)
			if (f->type == ft_prev)
				break;
		if (f)
			for (e2 = db->entries; e2; e2 = e2->next)
				if (f->len == strlen(e2->name) &&
				    !memcmp(f->data, e2->name, f->len)) {
					t->prev = e2;
					break;
				}
			/* ignore unmatched references */
		t++;
	}

	/*
	 * Based on Kahn's algorithm
	 * https://en.wikipedia.org/wiki/Topological_sorting#Kahn's_algorithm
	 */
	i = 0;
	while (1) {
		bool more = 0;
		bool progress = 0;

		for (t = tmp; t != tmp + n; t++) {
			if (!t->in)
				continue;
			if (t->prev) {
				more = 1;
			} else {
				progress = 1;
				tmp[i].out = t->in;
				i++;
				for (t2 = tmp; t2 != tmp + n; t2++)
					if (t2->prev == t->in)
						t2->prev = NULL;
				t->in = NULL;
			}
		}
		if (!more)
			break;

		/* break cycles */
		if (!progress)
			for (t = tmp; t != tmp + n; t++)
				if (t->in && t->prev) {
					t->prev = NULL;
					break;
				}
	}
	assert(i == n);

	/* apply new order */
	db->entries = tmp[0].out;
	for (t = tmp; t != tmp + n; t++)
		t->out->next = t + 1 == tmp + n ? NULL : t[1].out;

	free(tmp);

	return n;
}


/* --- Database entries: reordering ---------------------------------------- */


/*
 * is_prev determines if "prev" is the immediate predecessor of "e". If "prev"
 * is NULL, we return "1" if "e" has no predecessor.
 */

static bool is_prev(const struct db_entry *prev, const struct db_entry *e)
{
	const struct db_field *f = db_field_find(e, ft_prev);

	if (!prev && !f)
		return 1;
	if (!prev || !f)
		return 0;
	if (strlen(prev->name) != f->len)
		return 0;
	return !memcmp(prev->name, f->data, f->len);
}


void db_move_after(struct db_entry *e, const struct db_entry *after)
{
	struct db *db = e->db;
	struct db_entry *e2;
	struct db_field *f = db_field_find(e, ft_prev);

	if (after) {
if (debugging)
  printf("move_after: %s -> %s\n", e->name, after->name);
	} else {
if (debugging)
  printf("move_after: %s -> TOP\n", e->name);
	}

	/* @@@ is this the right place to handle moving after itself ? */
	if (e == after) {
if (debugging)
  printf("to self\n");
		return;
	}

if (debugging)
  printf("followers:\n");
	/*
	 * db_change_field and db_delete_entry sort the database, so we just
	 * restart to be safe.
	 */
again:
	f = db_field_find(e, ft_prev);
	for (e2 = db->entries; e2; e2 = e2->next)
		if (e2 != e && is_prev(e, e2)) {
			if (f) {
				db_change_field(e2, ft_prev, f->data, f->len);
			} else {
				struct db_field *f2 =
				    db_field_find(e2, ft_prev);

				assert(f2);
				db_delete_field(e2, f2);
			}
			goto again;
		}

if (debugging)
  printf("followers2:\n");
again2:
	for (e2 = db->entries; e2; e2 = e2->next)
{
if (debugging)
  printf("\t%s: %p =? %p, (%s) %u\n", e2->name, e2, e, e->name, is_prev(e, e2));
		if (e2 != e && is_prev(after, e2)) {
			db_change_field(e2, ft_prev, e->name, strlen(e->name));
			goto again2;
		}
}

	if (after) {
		db_change_field(e, ft_prev, after->name,
		    strlen(after->name));
	} else {
		if (f)
			db_delete_field(e, f);
	}

	db_tsort(db);
}


void db_move_before(struct db_entry *e, const struct db_entry *before)
{
	struct db *db = e->db;
	const struct db_entry *prev;

	if (before == db->entries) {
		prev = NULL;
	} else if (before) {
if (debugging)
  printf("move_before: %s -> %s\n", e->name, before->name);
		for (prev = db->entries; prev; prev = prev->next)
			if (prev->next == before)
				break;
		assert(prev);
	} else {
if (debugging)
  printf("move_before: %s -> END\n", e->name);
		for (prev = db->entries; prev && prev->next; prev = prev->next);
	}
	db_move_after(e, prev);
}


/* --- Database entries: deletion ------------------------------------------ */


static struct db_entry **find_anchor(struct db_entry **anchor,
    const struct db_entry *de)
{
	struct db_entry **found;

	while (*anchor) {
		if (*anchor == de)
			return anchor;
		if ((*anchor)->children) {
			found = find_anchor(&(*anchor)->children, de);
			if (found)
				return found;
		}
		anchor = &(*anchor)->next;
	}
	return NULL;
}


bool db_delete_entry(struct db_entry *de)
{
	struct db *db = de->db;
	struct db_entry **anchor;

	anchor = find_anchor(&db->entries, de);
	*anchor = de->next;

	db_tsort(db);

	if (!block_delete(de->block))
		return 0;
	span_add(&db->deleted, de->block, 1);
	free_entry(de);
	db->stats.data--;
	db->stats.deleted++;
	return 1;
}


/* --- Database entries: iteration ----------------------------------------- */


bool db_iterate(struct db *db, bool (*fn)(void *user, struct db_entry *de),
    void *user)
{
	struct db_entry *de, *next;

	for (de = db->dir ? db->dir->children : db->entries; de; de = next) {
		next = de->next;
		if (!fn(user, de))
			return 0;
	}
	return 1;
}


/* --- Directory operations ------------------------------------------------ */


bool db_is_dir(const struct db_entry *de)
{
	const struct db_field *f;

	for (f = de->fields; f; f = f->next)
		if (f->type == ft_dir)
			return 1;
	return !!de->children;
}


bool db_is_account(const struct db_entry *de)
{
	const struct db_field *f;

	if (de->children)
		return 0;
	for (f = de->fields; f; f = f->next)
		switch (f->type) {
		case ft_id:
		case ft_prev:
			break;
		case ft_dir:
			return 0;
		default:
			return 1;
		}
	return 0;
}


void db_mkdir(struct db_entry *de)
{
	assert(!db_is_dir(de));
	assert(!db_is_account(de));
	add_field(de, ft_dir, NULL, 0);
}


void db_chdir(struct db *db, struct db_entry *de)
{
	db->dir = de;
}


/*
 * @@@ We return NULL in two cases:
 * 1) if the parent is the root directory, and
 * 2) if we don't find "de" in the tree.
 * The latter should be a fatal error.
 */

static struct db_entry *find_parent(const struct db *db,
    struct db_entry *dir, const struct db_entry *de)
{
	struct db_entry *e, *parent;

	for (e = dir ? dir->children : db->entries; e; e = e->next) {
		if (e == de)
			return dir;
		parent = find_parent(db, e, db->dir);
		if (parent)
			return parent;
	}
	return NULL;
}


struct db_entry *db_dir_parent(const struct db *db)
{
	return find_parent(db, NULL, db->dir);
}


const char *db_pwd(const struct db *db)
{
	return db->dir ? db->dir->name : NULL;
}


/* --- Settings ------------------------------------------------------------ */


bool db_update_settings(struct db *db, uint16_t seq,
    const void *payload, unsigned length)
{
	int new;

	new = get_erased_block(db);
	if (new < 0)
		return 0;
	if (block_write(db->c, bt_settings, seq, payload, length, new)) {
		db->stats.special++;
		if (db->settings_block != -1) {
			if (block_delete(db->settings_block)) {
				span_add(&db->deleted, db->settings_block, 1);
				db->stats.deleted++;
				db->stats.special--;
			}
		}
		db->settings_block = new;
		return 1;
	} else {
		if (block_delete(new)) {
			span_add(&db->deleted, new, 1);
			db->stats.deleted++;
		}
		return 0;
	}
}


/* --- Fix remaining virtual entries --------------------------------------- */


/*
 * The database may contain some anomalies we fix here:
 * - entries in a directory for which no entry exists, or
 * - directory entries without the ft_dir field.
 * In both cases, we add an ft_dir field, so that other parts of the system do
 * not get confused about the status of an entry. (E.g., after deleting all
 * entries in a directory, it would look like an account record without fields
 * if it didn't have an ft_dir field.)
 */

static void fix_virtual(struct db_entry *dir)
{
	struct db_entry *e;

	for (e = dir; e; e = e->next) {
		fix_virtual(e->children);
		if (db_field_find(e, ft_dir))
			continue;
		/*
		 * @@@ note: we do not assign a block here. This only happens
		 * if we make changes to the directory entry.
		 */
		if (e->children || !e->block)
			add_field(e, ft_dir, NULL, 0);
	}
}


/* --- Open/close the account database ------------------------------------- */


void db_stats(const struct db *db, struct db_stats *s)
{
	*s = db->stats;
}


static const void *tlv_item(const void **p, const void *end,
    enum field_type *type, unsigned *len)
{
	const uint8_t *q = *p;

	if (*p + 2 > end)
		return NULL;
	if (*q == ft_end)
		return NULL;
	*type = *q++;
	*len = *q++;
	*p = q + *len;
	if (*p > end)
		return NULL;
	return q;
}


static char *alloc_string(const char *s, unsigned len)
{
	char *tmp;

	tmp = alloc_size(len + 1);
	memcpy(tmp, s, len);
	tmp[len] = 0;
	return tmp;
}


static bool process_payload(struct db *db, unsigned block, uint16_t seq,
    const void *payload, unsigned size)
{
	const void *end = payload + size;
	struct db_entry *de;
	const void *p, *q;
	enum field_type type;
	char *name;
	unsigned len;

	p = payload;
	q = tlv_item(&p, end, &type, &len);
	if (!q || type != ft_id)
		return 0;

	struct db_entry *dir = NULL;

	while (1) {
		const char *z;

		z = memchr(q, 0, len);
		if (!z)
			break;

		unsigned dir_len = z - (const char *) q;
		struct db_entry **a;

		for (a = dir ? &dir->children : &db->entries; *a;
		    a = &(*a)->next)
			if (!strcmp(q, (*a)->name))
				break;
		if (!*a) {
			/*
			 * virtual entry with block = 0
			 */
			de = alloc_type(struct db_entry);
			memset(de, 0, sizeof(*de));
			de->name = alloc_string(q, dir_len);
			de->db = db;
			*a = de;
		}
		dir = *a;
		len -= dir_len + 1;
		q = z + 1;
	}

	name = alloc_string(q, len);
	for (de = dir ? dir->children : db->entries; de; de = de->next)
		if (!strcmp(de->name, name))
			break;
	if (de) {
		free(name);
		if (de->block) {
			/*
			 * @@@ We lose blocks here. Should either have an
			 * "obsolete" type or consider it as "empty".
		 	*/
			if (((seq + 0x10000 - de->seq) & 0xffff) >= 0x8000)
				return 1;
			free_fields(de);
		} else {
			assert(!de->fields);
			assert(!de->seq);
		}
	} else {
		de = new_entry(db, dir, name);
		de->name = name;
	}
	de->block = block;
	de->seq = seq;
	p = payload;
	while (p != end) {
		q = tlv_item(&p, end, &type, &len);
		if (!q)
			break;
		add_field(de, type, q, len);
	}
	return 1;
}


void db_open_empty(struct db *db, const struct dbcrypt *c)
{
	memset(db, 0, sizeof(*db));
	db->c = c;
	db->generation = 0;
	db->stats.total = storage_blocks();
	db->entries = NULL;
	db->settings_block = -1;
	db->dir = NULL;
}


bool db_open_progress(struct db *db, const struct dbcrypt *c,
    void (*progress)(void *user, unsigned i, unsigned n), void *user)
{
	unsigned i;
	uint16_t seq;

	db_open_empty(db, c);
	for (i = RESERVED_BLOCKS; i != db->stats.total; i++) {
		unsigned payload_len = sizeof(payload_buf);

		if (progress)
			progress(user, i, db->stats.total);
		switch (block_read(c, &seq, payload_buf, &payload_len, i)) {
		case bt_error:
			db->stats.error++;
			break;
		case bt_deleted:
			span_add(&db->deleted, i, 1);
			db->stats.deleted++;
			break;
		case bt_erased:
			span_add(&db->erased, i, 1);
			db->stats.erased++;
			break;
		case bt_invalid:
			db->stats.invalid++;
			break;
		case bt_empty:
			span_add(&db->empty, i, 1);
			db->stats.empty++;
			break;
		case bt_data:
			if (process_payload(db, i, seq, payload_buf,
			    payload_len))
				db->stats.data++;
			else
				db->stats.invalid++;
			break;
		case bt_settings:
			if (settings_process(seq, payload_buf, payload_len)) {
				db->stats.special++;
				db->settings_block = i;
			} else {
				db->stats.invalid++;
			}
			break;
		default:
			ABORT();
		}
	}
	if (progress)
		progress(user, i, i);
	memset(payload_buf, 0, sizeof(payload_buf));
	db_tsort(db);
	fix_virtual(db->entries);
	db->dir = NULL;
	return 1;
}


bool db_open(struct db *db, const struct dbcrypt *c)
{
	return db_open_progress(db, c, NULL, NULL);
}


void db_close(struct db *db)
{
	while (db->entries) {
		struct db_entry *this = db->entries;

		db->entries = this->next;
		free_entry(this);
	}
	span_free_all(db->erased);
	span_free_all(db->deleted);
	span_free_all(db->empty);
}


bool db_is_erased(void)
{
	unsigned n = storage_blocks();
	unsigned i;

	/*
	 * @@@ We should tart at zero because also the presence of pad blocks
	 * is sufficient to indicate that the Flash is initialized.
	 */
	for (i = RESERVED_BLOCKS; i != n; i++) {
		enum block_type type = block_read(NULL, NULL, NULL, NULL, i);

		if (type != bt_error && type != bt_erased)
		    return 0;
	}
	return 1;
}


void db_init(void)
{
	unsigned i;

	assert(PAD_BLOCKS >= storage_erase_size() * 2);
	for (i = 0; i != sizeof(ft2order); i++)
		ft2order[order2ft[i]] = i;
}
