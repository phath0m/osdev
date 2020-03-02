/*
 * pool.c
 *
 * Allows for dynamic allocation of fixed size blocks. This is primarily being
 * to elevate some problems with the current malloc implementation in terms of
 * potentional deadlocks (There is one lock for malloc, so, if by chance, malloc
 * is pre-empted and something else tries to call malloc from a critical section,
 * a deadlock will occur.
 */
#include <ds/list.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/string.h>
#include <sys/systm.h>

struct pool_entry {
    void *  ptr;
};

void
pool_init(struct pool *pp, size_t size)
{
    memset(pp, 0, sizeof(struct pool));
    pp->entry_size = size;
}

void *
pool_get(struct pool *pp)
{
    void *ptr;

    if (!list_remove_front(&pp->free_items, &ptr)) {
        ptr = calloc(1, pp->entry_size);
        list_append(&pp->allocated_items, ptr);

        return ptr;
    }

    list_append(&pp->allocated_items, ptr);

    memset(ptr, 0, pp->entry_size);

    return ptr;
}

void
pool_put(struct pool *pp, void *ptr)
{
    if (!list_remove(&pp->allocated_items, ptr)) {
        panic("attempted to free invalid pointer");
    }

    list_append(&pp->free_items, ptr);
}
