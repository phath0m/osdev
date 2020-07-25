/*
 * pool.h
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
#ifndef _SYS_POOL_H
#define _SYS_POOL_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__
#include <ds/list.h>
#include <sys/mutex.h>
#include <sys/types.h>

struct pool {
    spinlock_t      lock;
    struct list     free_items;
    struct list     allocated_items;
    size_t          entry_size;
    uintptr_t       align;
};

void    pool_init(struct pool *, size_t, uintptr_t);
void *  pool_get(struct pool *);
void    pool_put(struct pool *, void *);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _SYS_POOL_H */
