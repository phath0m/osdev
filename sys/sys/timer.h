#ifndef _SYS_TIMER_H
#define _SYS_TIMER_H

#include <ds/list.h>
#include <sys/types.h>

struct timer;

typedef void (*timer_tick_t)(struct timer *timer, void *argp);

struct timer {
    timer_tick_t    handler;
    void *          argp;
    uint32_t        expires;
    bool            expired;
};

void timer_new(timer_tick_t handler, uint32_t timeout, void *argp);
void timer_expire(struct timer *timer);
void timer_renew(struct timer *timer, uint32_t newtimeout);
void timer_tick();

#endif
