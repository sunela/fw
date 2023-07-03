/*
 * timer.h - Timers and time handling
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef TIMER_H
#define	TIMER_H

struct timer {
	unsigned	due;
	void		(*fn)(void *user);
	void		*user;
	struct timer	*next;
};


/* @@@ now counts in ms, so it will overflow 32 bits after 49 days */
extern unsigned now;


void timer_init(struct timer *t);
void timer_set(struct timer *t, unsigned ms,
    void (*fn)(void *user), void *user);
void timer_cancel(struct timer *t);
void timer_flush(struct timer *t);

void timer_tick(unsigned ms);

#endif /* !TIMER_H */
