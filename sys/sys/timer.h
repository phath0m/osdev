/*
 * timer.h
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
#ifndef _ELYSIUM_SYS_TIMER_H
#define _ELYSIUM_SYS_TIMER_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__
#include <ds/list.h>
#include <sys/types.h>

struct timer;

typedef void (*timer_tick_t)(struct timer *, void *);

struct timer {
    timer_tick_t    handler;
    void *          argp;
    uint32_t        expires;
    bool            expired;
};

void timer_new(timer_tick_t, uint32_t, void *);
void timer_expire(struct timer *);
void timer_renew(struct timer *, uint32_t);
void timer_tick();

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _ELYSIUM_SYS_TIMER_H */
