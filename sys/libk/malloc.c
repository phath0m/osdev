/*
 * sys/rtl/malloc.h
 * Cross platform malloc implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mutex.h>
#include <sys/types.h>

#define MALLOC_ALIGNMENT 256

struct malloc_block {
    void    *prev;
    void    *ptr;
    size_t  size;
};


intptr_t kernel_break;
intptr_t kernel_heap_end;

struct malloc_block *last_allocated = NULL;
struct malloc_block *last_freed = NULL;

spinlock_t malloc_lock;

/*
static inline size_t
align_addr(size_t size, uint32_t align)
{
    return (size + align) - (size + align) % align;
}*/

static struct malloc_block *
find_free_block(size_t size)
{
    struct malloc_block *iter = last_freed;
    struct malloc_block *prev = NULL;

    while (iter) {
        if (iter->size == size) {
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
calloc(int num, size_t size)
{
    void *ptr = malloc(size);

    memset(ptr, num, size);

    return ptr;
}

void *
malloc(size_t size)
{
    spinlock_lock(&malloc_lock);

    size_t aligned_size = (size + MALLOC_ALIGNMENT) & 0xFFFFFF00;

    struct malloc_block *free_block = find_free_block(aligned_size);

    if (!free_block) {
        free_block = (struct malloc_block*)sbrk(sizeof(struct malloc_block));
        free_block->ptr = sbrk(size);
        free_block->size = aligned_size;
    }

    free_block->prev = last_allocated;
    last_allocated = free_block;

    spinlock_unlock(&malloc_lock);
    memset(free_block->ptr, 0, size);

    return free_block->ptr;
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
    kernel_break = (uintptr_t)ptr + MALLOC_ALIGNMENT;
    kernel_break &= 0xFFFFFF00;
    last_allocated = NULL;
    last_freed = NULL;
    
    kernel_heap_end = kernel_break + 0x1000000;
    
    spinlock_unlock(&malloc_lock);
    return 0; 
}

void *
sbrk(size_t increment)
{
    kernel_break += MALLOC_ALIGNMENT;
    kernel_break &= 0xFFFFFF00;

    intptr_t prev_brk = kernel_break;

    kernel_break += MALLOC_ALIGNMENT + increment;
    kernel_break &= 0xFFFFFF00;

    memset((void*)prev_brk, 0, increment);

    if (kernel_break >= kernel_heap_end) {
        printf("full stop! prev_brk=%p kernel_break=%p\n", prev_brk, kernel_break);
        asm volatile("cli");
        asm volatile("hlt");
    }

    return (void*)prev_brk; 
}
