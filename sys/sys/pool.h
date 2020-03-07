#ifndef _SYS_POOL_H
#define _SYS_POOL_H

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

void    pool_init(struct pool *pp, size_t size, uintptr_t align);
void *  pool_get(struct pool *pp);
void    pool_put(struct pool *pp, void *ptr);

#endif
