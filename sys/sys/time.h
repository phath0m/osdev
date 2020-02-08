#ifndef _TIME_H
#define _TIME_H

#include <sys/types.h>

struct timeval {
	long tv_sec;
	long tv_usec;
};

time_t time(time_t *tmloc);

#endif
