#ifndef _TIME_H
#define _TIME_H

#include <sys/types.h>

struct timeval {
	long tv_sec;
	long tv_usec;
};

extern time_t           time_second; // seconds since boot
extern struct timeval   time_delta; // delta set via adjtime()

time_t  time(time_t *tmloc);
int     adjtime(const struct timeval *newval, struct timeval *old);
int     gettimeofday(struct timeval *tv);

#endif
