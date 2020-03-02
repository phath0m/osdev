#ifndef _SYS_POOL_H
#define _SYS_POOL_H

#include <ds/list.h>
#include <sys/types.h>

struct pool {
    struct list     free_items;
    struct list     allocated_items;
    size_t          entry_size;
};

void    pool_init(struct pool *pp, size_t size);
void *  pool_get(struct pool *pp);
void    pool_put(struct pool *pp, void *ptr);

#endif
