/*
 * timer.c - Timers and time handling
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "timer.h"


unsigned now;

static struct timer *timers = NULL;


void timer_set(struct timer *t, unsigned ms,
    void (*fn)(void *user), void *user)
{
	unsigned due = now + ms;
	struct timer **anchor;

	if (t->fn)
		timer_cancel(t);
	for (anchor = &timers; *anchor; anchor = &(*anchor)->next)
		if ((*anchor)->due > due)
			break;
	t->next = *anchor;
	*anchor = t;
	t->due = due;
	t->fn = fn;
	t->user = user;
}


void timer_cancel(struct timer *t)
{
	struct timer **anchor;

	for (anchor = &timers; *anchor; anchor = &(*anchor)->next)
		if (*anchor == t) {
			*anchor = t->next;
			timer_init(t);
			break;
		}
}


void timer_flush(struct timer *t)
{
	struct timer **anchor;

	for (anchor = &timers; *anchor; anchor = &(*anchor)->next)
		if (*anchor == t) {
			void (*fn)(void *user) = t->fn;
			void *user = t->user;

			assert(fn);
			*anchor = t->next;
			timer_init(t);
			fn(user);
			break;
		}
}


void timer_init(struct timer *t)
{
	memset(t, 0, sizeof(*t));
}


void timer_tick(unsigned now_ms)
{
	now = now_ms;
	while (timers && timers->due <= now) {
		struct timer *t = timers;
		void (*fn)(void *user) = t->fn;
		void *user = t->user;

		timers = t->next;
		timer_init(t);
		fn(user);
	}
}
