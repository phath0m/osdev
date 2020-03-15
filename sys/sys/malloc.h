/*
 * malloc.h
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
#ifndef _SYS_MALLOC_H
#define _SYS_MALLOC_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__
#include <sys/types.h>

static inline size_t
align_addr(size_t size, uint32_t align)
{
    return (size + align) - (size + align) % align;
}

void *  calloc(int num, size_t size);
void *  malloc(size_t size);
void    free(void *ptr);
int     brk(void *ptr);
void *  sbrk(size_t increment);
void *  sbrk_a(size_t increment, uintptr_t align);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* __SYS_MALLOC_H */
