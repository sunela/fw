/*
 * span.h - Lists of spans (consecutive blocks)
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SPAN_H
#define	SPAN_H

struct db_span;


void span_add(struct db_span **spans, unsigned n, unsigned size);
int span_pull_one(struct db_span **spans);
int span_pull_erase_block(struct db_span **spans, unsigned erase_size);
void span_free_all(struct db_span *spans);

#endif /* !SPAN_H */
