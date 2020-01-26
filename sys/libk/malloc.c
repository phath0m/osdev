/*
 * sys/rtl/malloc.h
 * Cross platform malloc implementation
 */

#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>

#define MALLOC_ALIGNMENT 128
#define ALIGN_MASK 0xFFFFFF80

struct malloc_block {
    uint32_t    magic;
    void    *   prev;
    void    *   ptr;
    size_t      size;
};

#define HEAP_MAGIC 0xBADB01

intptr_t kernel_break;
intptr_t kernel_heap_start;
intptr_t kernel_heap_end;
int      kernel_heap_allocated_blocks = 0;
int      kernel_heap_free_blocks = 0;

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
        
        if (iter->magic != HEAP_MAGIC) {
            panic("heap corruption detected");
        }

        if (iter->size == size) {
            if (prev) {
                prev->prev = iter->prev;
            } else {
                last_freed = iter->prev;
            }
            kernel_heap_free_blocks--;
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

    size_t aligned_size = (size + MALLOC_ALIGNMENT) & ALIGN_MASK;

    struct malloc_block *free_block = find_free_block(aligned_size);

    if (!free_block) {
        free_block = (struct malloc_block*)sbrk(sizeof(struct malloc_block));
        free_block->magic = HEAP_MAGIC;
        free_block->ptr = sbrk(size);
        free_block->size = aligned_size;
    }

    free_block->prev = last_allocated;
    last_allocated = free_block;

    kernel_heap_allocated_blocks++;

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
    bool block_freed = false;

    while (iter) {

        if (iter->ptr == ptr) {
            if (prev) {
                prev->prev = iter->prev;
            } else {
                last_allocated = iter->prev;
            }

            iter->prev = last_freed;
            last_freed = iter;
            kernel_heap_allocated_blocks--;
            kernel_heap_free_blocks++;
            block_freed = true;
            break;
        }

        prev = iter;
        iter = (struct malloc_block*)iter->prev;
    }
    
    spinlock_unlock(&malloc_lock);
    
    if (!block_freed) {
        stacktrace(5);
        panic("double free (0x%p)", ptr);
    }
}


int
brk(void *ptr)
{
    kernel_break = (uintptr_t)ptr + 256;
    kernel_break &= 0xFFFFFF00;
    last_allocated = NULL;
    last_freed = NULL;
    
    kernel_heap_end = kernel_break + 0x1FC00000;
    kernel_heap_start = kernel_break;
    
    spinlock_unlock(&malloc_lock);

    return 0; 
}

void *
sbrk(size_t increment)
{
    intptr_t prev_brk = kernel_break;

    kernel_break += increment;
    kernel_break += MALLOC_ALIGNMENT*2;
    kernel_break &= ALIGN_MASK;

    //memset((void*)prev_brk, 0, increment);

    if (kernel_break >= kernel_heap_end) {
        panic("kernel heap full");
    }

    return (void*)prev_brk; 
}

