/*
 * timer.c - Kernel time implementation
 *
 * Allows code in kernel space to register a function that will be called after
 * a set amount of time.
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
#include <ds/list.h>
#include <sys/malloc.h>
#include <sys/sched.h>
#include <sys/string.h>
#include <sys/time.h>
#include <sys/timer.h>

time_t          time_second;
struct timeval  time_delta;
struct list     timer_list;

int
adjtime(const struct timeval *delta, struct timeval *olddelta)
{
    if (olddelta) {
        memcpy(olddelta, &time_delta, sizeof(struct timeval));
    }

    if (delta) {
        memcpy(&time_delta, delta, sizeof(struct timeval));
    }

    return 0;
}   

time_t
time(time_t *tmlock)
{
    if (tmlock) {
        *tmlock = time_second + time_delta.tv_sec;
    }

    return time_second + time_delta.tv_sec;
}

void
timer_new(timer_tick_t handler, uint32_t timeout, void *argp)
{
    struct timer *timer;
    
    timer = calloc(1, sizeof(struct timer));
    timer->expired = false;
    timer->expires = sched_ticks + timeout;
    timer->handler = handler;
    timer->argp = argp;
    
    list_append(&timer_list, timer);
}

void
timer_expire(struct timer *timer)
{
    list_remove(&timer_list, timer);
    free(timer);
}

void
timer_renew(struct timer *timer, uint32_t new_timeout)
{
    timer->expired = false;
    timer->expires = sched_ticks + new_timeout;
}

void
timer_tick()
{
    list_iter_t iter;
    struct timer *timer;

    time_second = sched_ticks / sched_hz;

    list_get_iter(&timer_list, &iter);

    while (iter_move_next(&iter, (void**)&timer)) {
        if (!timer->expired && timer->expires < sched_ticks) {
            timer->expired = true;
            timer->handler(timer, timer->argp);

            if (timer->expired) {
                timer_expire(timer);
            }
        }
    }

    iter_close(&iter);
}
