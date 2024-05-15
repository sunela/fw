/*
 * span.c - Lists of spans (consecutive blocks)
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>

#include "alloc.h"
#include "span.h"


struct db_span {
	unsigned start;
	unsigned len;
	struct db_span *next;
};


void span_add(struct db_span **spans, unsigned n, unsigned size)
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


int span_pull_one(struct db_span **spans)
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


int span_pull_erase_block(struct db_span **spans, unsigned erase_size)
{
	struct db_span *this;
	unsigned n;

	while (*spans) {
		this = *spans;
		n = round_up(this->start, erase_size);
		if (this->start + this->len >= n + erase_size)
			break;
		spans = &(*spans)->next;
	}
	if (!*spans)
		return -1;
	n = round_up(this->start, erase_size);
	if (n == this->start) {
		if (this->len == erase_size) {
			*spans = this;
			free(this);
		} else {
			this->start += erase_size;
			this->len -= erase_size;
		}
	} else {
		if (this->start + this->len == n + erase_size) {
			this->len -= erase_size;
		} else {
			struct db_span *next;

			next = alloc_type(struct db_span);
			next->start = n + erase_size;
			next->len = this->start + this->len - n - erase_size;
			next->next = this->next;
			this->len = n - this->start;
			this->next = next;
		}
	}
	return n;
}


void span_free_all(struct db_span *spans)
{
	while (spans) {
		struct db_span *next = spans->next;

		free(spans);
		spans = next;
	}
}
