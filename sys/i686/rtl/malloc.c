#include <stdlib.h>
#include <string.h>
#include <sys/mutex.h>
#include <sys/i686/vm.h>

/*
 * redefined, original definition in sys/rtl/malloc.c
 */
struct malloc_block {
    void *  prev;
    void *  ptr;
    size_t  size;
};

/*
 * defined in sys/rtl/malloc.c
 */
extern spinlock_t malloc_lock;
extern uintptr_t kernel_break;
extern struct malloc_block *last_allocated;
extern struct malloc_block *last_freed;

static struct malloc_block *
find_free_pa_block(size_t size)
{
    struct malloc_block *iter = last_freed;
    struct malloc_block *prev = NULL;
    while (iter) {
        if (iter->size == size && (((uint32_t)iter->ptr & 0xFFF) == 0)) {
            
            if (prev) {
                prev->prev = iter->prev;
            } else {
                last_freed = iter->prev;
            }

            return iter;
        }
        prev = iter;
        iter = (struct malloc_block*)iter->prev;
    }

    return NULL;
}

void *
malloc_pa(size_t size)
{
    spinlock_lock(&malloc_lock);

    size_t aligned_size = (size + PAGE_SIZE) & 0xFFFFF000;

    struct malloc_block *free_block = find_free_pa_block(aligned_size);

    if (!free_block) {
        free_block = (struct malloc_block*)sbrk(sizeof(struct malloc_block));
        free_block->ptr = sbrk_a(aligned_size);
        free_block->size = aligned_size;
    }

    free_block->prev = last_allocated;
    last_allocated = free_block;

    spinlock_unlock(&malloc_lock);

    memset(free_block->ptr, 0, size);

    return free_block->ptr;
}

void *
sbrk_a(size_t increment)
{
    kernel_break += 0x1000;
    kernel_break &= 0xFFFFF000;

    intptr_t prev_brk = kernel_break;

    kernel_break += 0x1000 + increment;
    kernel_break &= 0xFFFFF000;

    memset((void*)prev_brk, 0, increment);

    return (void*)prev_brk;
}
