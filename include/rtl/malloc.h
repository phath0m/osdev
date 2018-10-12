#ifndef RTL_MALLOC_H
#define RTL_MALLOC_H

#include <rtl/types.h>

static inline size_t
align_addr(size_t size, uint32_t align)
{
    return (size + align) - (size + align) % align;
}

void *calloc(int num, size_t size);
void *malloc(size_t size);
void *malloc_pa(size_t size);
void free(void *ptr);

int brk(void *ptr);
void *sbrk(size_t increment);
void *sbrk_a(size_t increment);

#endif
