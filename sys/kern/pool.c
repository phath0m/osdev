/*
 * pool.c
 *
 * Allows for dynamic allocation of fixed size blocks of memory. 
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
#include <sys/pool.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/string.h>
#include <sys/systm.h>

void
pool_init(struct pool *pp, size_t size, uintptr_t align)
{
    memset(pp, 0, sizeof(struct pool));
    pp->entry_size = size;
    pp->align = align;
}

void *
pool_get(struct pool *pp)
{
    void *ptr;

    spinlock_lock(&pp->lock);

    if (!list_remove_front(&pp->free_items, &ptr)) {    
        if (pp->align != 0) {
            ptr = sbrk_a(pp->entry_size, pp->align);
        } else {
            ptr = sbrk(pp->entry_size);
        }

        list_append(&pp->allocated_items, ptr);

        goto _cleanup;
    }

    list_append(&pp->allocated_items, ptr);

_cleanup:
    memset(ptr, 0, pp->entry_size);
    spinlock_unlock(&pp->lock);

    return ptr;
}

void
pool_put(struct pool *pp, void *ptr)
{
    spinlock_lock(&pp->lock);

    if (!list_remove(&pp->allocated_items, ptr)) {
        panic("attempted to free invalid pointer");
    }

    list_append(&pp->free_items, ptr);
    
    spinlock_unlock(&pp->lock);
}
