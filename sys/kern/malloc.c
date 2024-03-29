/*
 * malloc.c - kernel heap implementation
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
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>

#define MALLOC_ALIGNMENT    128
#define ALIGN_MASK             0xFFFFFF80

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

/*
static inline size_t
align_addr(size_t size, uint32_t align)
{
    return (size + align) - (size + align) % align;
}*/

static struct malloc_block *
find_free_block(size_t size)
{
    struct malloc_block *iter;
    struct malloc_block *prev;

    iter = last_freed;
    prev = NULL;

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
        iter = iter->prev;
    }

    return NULL;
}

void *
calloc(int num, size_t size)
{
    void *ptr;
    
    ptr = malloc(size);

    memset(ptr, num, size);

    return ptr;
}

void *
malloc(size_t size)
{
    size_t aligned_size;
    struct malloc_block *free_block;

    critical_enter();

    aligned_size = (size + MALLOC_ALIGNMENT) & ALIGN_MASK;
    free_block = find_free_block(aligned_size);

    if (!free_block) {
        free_block = (struct malloc_block*)sbrk(sizeof(struct malloc_block));
        free_block->magic = HEAP_MAGIC;
        free_block->ptr = sbrk(size);
        free_block->size = aligned_size;
    }

    free_block->prev = last_allocated;
    last_allocated = free_block;

    kernel_heap_allocated_blocks++;

    critical_exit();

    memset(free_block->ptr, 0, size);

    return free_block->ptr;
}

void
free(void *ptr)
{
    bool block_freed;

    struct malloc_block *iter;
    struct malloc_block *prev;

    critical_enter();

    iter = last_allocated;
    prev = NULL;
    block_freed = false;

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
        iter = iter->prev;
    }
    
    //spinlock_unlock(&malloc_lock);
    critical_exit();

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
    
    return 0; 
}

void *
sbrk(size_t increment)
{
    intptr_t prev_brk;
    
    prev_brk = kernel_break;

    kernel_break += increment;
    kernel_break += MALLOC_ALIGNMENT*2;
    kernel_break &= ALIGN_MASK;

    //memset((void*)prev_brk, 0, increment);

    if (kernel_break >= kernel_heap_end) {
        panic("kernel heap full");
    }

    return (void*)prev_brk; 
}

void *
sbrk_a(size_t increment, uintptr_t align)
{
    int chunks_allocated;
    uintptr_t align_mask;
    uintptr_t prev_brk;

    struct malloc_block *free_block;

    align_mask = align - 1;
    chunks_allocated = 0;

    while ((kernel_break & align_mask) != 0) {
        free_block = sbrk(sizeof(struct malloc_block));
        free_block->magic = HEAP_MAGIC;
        free_block->ptr = sbrk(128);
        free_block->size = 128;
        free_block->prev = last_freed;
        last_freed = free_block;
        kernel_heap_free_blocks++;
        chunks_allocated++;

        if (chunks_allocated > 128) {
            panic("Okay, this isn't going to work!\n");
        }
    }

    prev_brk = kernel_break;

    if (increment & align_mask) {
        increment += align;
    }

    kernel_break += increment;
    kernel_break &= ~(align_mask);

    memset((void*)prev_brk, 0, increment);

    return (void*)prev_brk;
}
