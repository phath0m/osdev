#ifndef _SYS_TIME_H
#define _SYS_TIME_H

#include <sys/types.h>

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

struct timeval {
	long tv_sec;
	long tv_usec;
};

extern time_t           time_second; // seconds since boot
extern struct timeval   time_delta; // delta set via adjtime()

time_t  time(time_t *tmloc);
int     adjtime(const struct timeval *newval, struct timeval *old);

#endif
