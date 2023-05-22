#ifndef TIMER_H
#define	TIMER_H

struct timer {
	unsigned	due;
	void		(*fn)(void *user);
	void		*user;
	struct timer	*next;
};


extern unsigned now;


void timer_init(struct timer *t);
void timer_set(struct timer *t, unsigned ms,
    void (*fn)(void *user), void *user);
void timer_cancel(struct timer *t);
void timer_flush(struct timer *t);

void timer_tick(unsigned ms);

#endif /* !TIMER_H */
