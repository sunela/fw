/*
 * db.c - Account database
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "debug.h"
#include "alloc.h"
#include "storage.h"
#include "block.h"
#include "db.h"


struct db_span {
	unsigned start;
	unsigned len;
	struct db_span *next;
};


static PSRAM uint8_t payload_buf[BLOCK_PAYLOAD_SIZE];


/* --- Spans --------------------------------------------------------------- */


static void span_add(struct db_span **spans, unsigned n, unsigned size)
{
	struct db_span *this;

	while (*spans) {
		this = *spans;
		if (this->start + this->len == n) {
			struct db_span *next = this->next;

			this->len += size;
			if (!next)
				return;
			if (next->start != this->start + this->len)
				return;
			this->len += next->len;
			this->next = next->next;
			free(next);
			return;
		}
		if (this->start == n + size) {
			this->start -= size;
			this->len += size;
			break;
		}
		spans = &(*spans)->next;
	}
	*spans = this = alloc_type(struct db_span);
	this->start = n;
	this->len = size;
	this->next = NULL;
}


static int span_pull_one(struct db_span **spans)
{
	struct db_span *this = *spans;
	unsigned n;

	if (!this)
		return -1;
	n = this->start;
	if (--this->len) {
		this->start++;
	} else {
		*spans = this->next;
		free(this);
	}
	return n;
}


static inline unsigned round_up(unsigned n, unsigned mod)
{
	n += mod - 1;
	return n - n % mod;
}


static int span_pull_erase_block(struct db_span **spans)
{
	unsigned size = storage_erase_size();
	struct db_span *this;
	unsigned n;

	while (*spans) {
		this = *spans;
		n = round_up(this->start, size);
		if (this->start + this->len >= n + size)
			break;
		spans = &(*spans)->next;
	}
	if (!*spans)
		return -1;
	n = round_up(this->start, size);
	if (n == this->start) {
		if (this->len == size) {
			*spans = this;
			free(this);
		} else {
			this->start += size;
			this->len -= size;
		}
	} else {
		if (this->start + this->len == n + size) {
			this->len -= size;
		} else {
			struct db_span *next;

			next = alloc_type(struct db_span);
			next->start = n + size;
			next->len = this->start + this->len - n - size;
			next->next = this->next;
			this->len = n - this->start;
			this->next = next;
		}
	}
	return n;
}


static void free_spans(struct db_span *spans)
{
	while (spans) {
		struct db_span *next = spans->next;

		free(spans);
		spans = next;
	}
}


/* --- Get an erased block, erasing if needed ------------------------------ */


static int get_erased_block(struct db *db)
{
	unsigned size = storage_erase_size();
	int n;

	while (1) {
		n = span_pull_one(&db->erased);
		if (n >= 0)
			break;
		n = span_pull_erase_block(&db->empty);
		if (n >= 0) {
			db->stats.empty -= size;
			if (storage_erase_blocks(n, size)) {
				db->stats.erased += size;
				continue;
			}
			db->stats.error += size;
		}
		n = span_pull_erase_block(&db->deleted);
		if (n >= 0) {
			db->stats.deleted -= size;
			if (storage_erase_blocks(n, size)) {
				db->stats.erased += size;
				continue;
			}
			db->stats.error += size;
		}
		return -1;
	}
	return n;
}


/* --- Helper functions ---------------------------------------------------- */


static void free_fields(struct db_entry *de)
{
	while (de->fields) {
		struct db_field *this = de->fields;

		de->fields = this->next;
		free(this->data);
		free(this);
	}
}


/* --- Database entries ---------------------------------------------------- */


static struct db_entry *new_entry(struct db *db, const char *name,
    const char *prev)
{
	struct db_entry *de, **anchor;
	struct db_entry *e = NULL;

	de = alloc_type(struct db_entry);
	memset(de, 0, sizeof(*de));
	de->db = db;
	if (prev)
		for (e = db->entries; e; e = e->next)
			if (!strcmp(e->name, prev))
				break;
	if (e) {
		anchor = &e->next;
	} else {
		for (anchor = &db->entries; *anchor; anchor = &(*anchor)->next)
			if (strcmp((*anchor)->name, name) > 0)
				break;
	}
	de->next = *anchor;
	*anchor = de;
	return de;
}


struct db_entry *db_new_entry(struct db *db, const char *name)
{
	return NULL;
}


static bool write_entry(const struct db_entry *de)
{
	const void *end = payload_buf + sizeof(payload_buf);
	uint8_t *p = payload_buf;
	const struct db_field *f;

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
	return block_write(de->db->c, ct_data, de->seq, payload_buf, de->block);
}


bool db_change(struct db_entry *de, enum field_type type,
    const void *data, unsigned size)
{
	struct db *db = de->db;
	struct db_field **anchor;
	struct db_field *f;
	unsigned old = de->block;
	int new;

	new = get_erased_block(de->db);
	if (new < 0)
		return 0;
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
	if (block_delete(old)) {
		span_add(&db->deleted, old, 1);
		db->stats.deleted++;
	}
	return 1;
}


bool db_delete(struct db_entry *de)
{
	// @@@
	return 0;
}


bool db_iterate(struct db *db, bool (*fn)(void *user, struct db_entry *de),
    void *user)
{
	struct db_entry *de, *next;

	for (de = db->entries; de; de = next) {
		next = de->next;
		if (!fn(user, de))
			return 0;
	}
	return 1;
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
	struct db_field **df;
	const void *p, *q;
	enum field_type type;
	char *name;
	unsigned len;

	p = payload;
	q = tlv_item(&p, end, &type, &len);
	if (!q || type != ft_id)
		return 0;
	name = alloc_string(q, len);
	for (de = db->entries; de; de = de->next)
		if (!strcmp(de->name, name))
			break;
	if (de) {
		free(name);
		/*
		 * @@@ We lose blocks here. Should either have an "obsolete"
		 * type or consider it as "empty".
		 */
		if (((seq + 0x10000 - de->seq) & 0xffff) >= 0x8000)
			return 1;
		free_fields(de);
	} else {
		char *prev = NULL;

		while (1) {
			q = tlv_item(&p, end, &type, &len);
			if (!q)
				break;
			if (type == ft_prev) {
				prev = alloc_string(q, len);
				break;
			}
		}
		de = new_entry(db, name, prev);
		de->name = name;
		free(prev);
	}
	de->block = block;
	de->seq = seq;
	df = &de->fields;
	p = payload;
	while (p != end) {
		q = tlv_item(&p, end, &type, &len);
		if (!q)
			break;
		*df = alloc_type(struct db_field);
		(*df)->type = type;
		(*df)->len = len;
		(*df)->data = alloc_size(len);
		memcpy((*df)->data, q, len);
		(*df)->next = NULL;
		df = &(*df)->next;
	}
	return 1;
}


bool db_open(struct db *db, const struct dbcrypt *c)
{
	unsigned i;
	uint16_t seq;

	memset(db, 0, sizeof(*db));
	db->c = c;
	db->stats.total = storage_blocks();
	db->entries = NULL;
	for (i = 0; i != db->stats.total; i++)
		switch (block_read(c, &seq, payload_buf, i)) {
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
			    BLOCK_PAYLOAD_SIZE))
				db->stats.data++;
			else
				db->stats.invalid++;
			break;
		default:
			abort();
		}
	memset(payload_buf, 0, sizeof(payload_buf));
	return 1;
}


void db_close(struct db *db)
{
	while (db->entries) {
		struct db_entry *this = db->entries;

		db->entries = this->next;
		free((void *) this->name);
		free_fields(this);
		free(this);
	}
	free_spans(db->erased);
	free_spans(db->deleted);
	free_spans(db->empty);
}
