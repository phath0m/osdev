#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <stdlib.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

#define POINT_LESS_THAN(x1,y1,x2,y2) (y1 < y2 && x1 < x2)
#define POINT_GREATER_THAN(x1,y1,x2,y2) (y1 > y2 && x2 > x1)

static inline void
fast_memcpy_d(void *dst, const void *src, size_t nbyte)
{
    asm volatile("cld\n\t"
                 "rep ; movsd"
                 : "=D" (dst), "=S" (src)
                 : "c" (nbyte / 4), "D" (dst), "S" (src)
                 : "memory");
}

static inline void
fast_memset_d(void *dst, uint32_t val, size_t nbyte)
{
    asm volatile("cld; rep stosl\n\t"
            : "+c"(nbyte), "+D" (dst) : "a" (val)
            : "memory");
}   

#endif
