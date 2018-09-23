#include <rtl/malloc.h>
#include <rtl/mutex.h>
#include <rtl/types.h>

#define MALLOC_ALIGNMENT 256

struct malloc_block {
    void    *prev;
    void    *ptr;
    size_t  size;
};


static intptr_t kernel_break;

static struct malloc_block *last_allocated = NULL;
static struct malloc_block *last_freed = NULL;

static spinlock_t malloc_lock;

static inline size_t
align_malloc_size(size_t size)
{
    return (size + MALLOC_ALIGNMENT) - (size + MALLOC_ALIGNMENT) % MALLOC_ALIGNMENT;
}

static struct malloc_block *
find_free_block(size_t size)
{
    struct malloc_block *iter = last_freed;
    
    size_t aligned_size = align_malloc_size(size);

    while (iter) {
        if (iter->size == aligned_size) {
            return iter;
        }

        iter = (struct malloc_block*)iter->prev;
    }

    return NULL;
}

void *
malloc(size_t size)
{
    spinlock_lock(&malloc_lock);

    struct malloc_block *free_block = find_free_block(size);

    if (free_block) {
        spinlock_unlock(&malloc_lock);
        return free_block->ptr;
    }

    size_t aligned_size = align_malloc_size(size);

    struct malloc_block *new_block = (struct malloc_block*)sbrk(sizeof(struct malloc_block));

    void *ptr = sbrk(aligned_size);

    new_block->ptr = ptr;
    new_block->size = aligned_size;
    new_block->prev = last_allocated;

    last_allocated = new_block;

    spinlock_unlock(&malloc_lock);

    return ptr;
}

void
free(void *ptr)
{
    spinlock_lock(&malloc_lock);

    struct malloc_block *iter = last_allocated;
    struct malloc_block *prev = NULL;

    while (iter) {

        if (iter->ptr == ptr) {
            if (prev) {
                prev->prev = iter->prev;
            } else {
                last_allocated = iter->prev;
            }

            iter->prev = last_freed;
            
            last_freed = iter;
            
            break;
        }

        prev = iter;
        iter = (struct malloc_block*)iter->prev;
    }

    spinlock_unlock(&malloc_lock);
}


int
brk(void *ptr)
{
    kernel_break = (intptr_t)ptr;

    return 0; 
}

void *
sbrk(size_t increment)
{
    intptr_t prev_brk = kernel_break;

    brk((void*)(kernel_break + increment));

    return (void*)prev_brk;
}
