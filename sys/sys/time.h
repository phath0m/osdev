/*
 * time.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _ELYSIUM_SYS_TIME_H
#define _ELYSIUM_SYS_TIME_H

#include <sys/types.h>

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

struct timeval {
	long tv_sec;
	long tv_usec;
};

#ifdef __KERNEL__
extern time_t           time_second; // seconds since boot
extern struct timeval   time_delta; // delta set via adjtime()
#endif 

time_t  time(time_t *);
int     adjtime(const struct timeval *, struct timeval *);

#endif
