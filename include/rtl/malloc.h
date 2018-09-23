#ifndef RTL_MALLOC_H
#define RTL_MALLOC_H

#include <rtl/types.h>

void *malloc(size_t size);
void free(void *ptr);

int brk(void *ptr);
void *sbrk(size_t increment);

#endif
