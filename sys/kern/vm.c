/*
 * vm.c - machine independent virtual memory code
 *
 * Really, all this file is responsible for is controlling allocation of virtual
 * address. Everything else of note is currently in the architecture dependent
 * memory manager (sys/<ARCH>/kern/vm.c)
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
#include <machine/vm.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vm.h>

/* bitmap macros */
#define BITMAP_INDEX(n) ((n) >> 3)
#define BITMAP_BIT(n) ((n) & 7)
#define BITMAP_SIZE(n) (((n) >> 3) + 1)
#define BITMAP_SET(b, n) (((uint8_t*)b)[BITMAP_INDEX(n)] |= (1 << BITMAP_BIT(n)))
#define BITMAP_CLEAR(b, n) (((uint8_t*)b)[BITMAP_INDEX(n)] &= ~(1 << BITMAP_BIT(n)))
#define BITMAP_GET(b, n) (((uint8_t*)b)[BITMAP_INDEX(n)] & (1 << BITMAP_BIT(n)))

struct va_map *
va_map_new(uintptr_t base, uintptr_t limit)
{
    size_t bitmap_size;
    struct va_map *vamap;
 
    bitmap_size = PAGE_COUNT(limit - base);
    vamap = calloc(1, sizeof(struct va_map) + bitmap_size);

    vamap->base = base;
    vamap->limit = limit;

    return vamap;
}

/* finds a suitable virtual address */
uintptr_t
va_find_block(struct va_map *vamap, size_t length)
{
    int i;
    int free_page_count;

    size_t required_pages;
    size_t max_pages;
    uint8_t *bitmap;

    free_page_count = 0;
    required_pages = PAGE_COUNT(length);
    max_pages = PAGE_COUNT(vamap->limit);
    bitmap = vamap->bitmap;

    for (i = 1; i < max_pages; i++) {
        if (free_page_count == required_pages) {
            return vamap->base + (i - free_page_count) * 4096;
        }

        if (BITMAP_GET(bitmap, i)) {
            free_page_count = 0;
        } else {
            free_page_count++;
        }
    }

    panic("out of memory!");

    return 0;
}

/* marks a block of virtual addresses as "allocated" */
void
va_mark_block(struct va_map *vamap, uintptr_t addr, size_t length)
{
    int i;
    int start_page;
    size_t required_pages;
    uint8_t *bitmap;

    if (addr + length > vamap->limit) {
        return;
    }

    addr -= vamap->base;

    required_pages = PAGE_COUNT(length);
    start_page = PAGE_INDEX(addr);
    bitmap = vamap->bitmap;

    for (i = 0; i < required_pages; i++) {
        BITMAP_SET(bitmap, start_page + i);
    }
}

/* allocate an used virtual address */
uintptr_t
va_alloc_block(struct va_map *vamap, uintptr_t addr, size_t length)
{
    if (addr == 0) {
        addr = va_find_block(vamap, length);
    }

    va_mark_block(vamap, addr, length);

    return addr;
}

/* marks a block of virtual addresses as "free" */
void
va_free_block(struct va_map *vamap, uintptr_t addr, size_t length)
{
    int i;
    int start_page;
    size_t required_pages;
    uint8_t *bitmap;

    if (addr + length > vamap->limit) {
        return;
    }

    addr -= vamap->base;

    required_pages = PAGE_COUNT(length);
    start_page = PAGE_INDEX(addr);

    bitmap = vamap->bitmap;

    for (i = 0; i < required_pages; i++) {
        BITMAP_CLEAR(bitmap, start_page + i);
    }
}

/* validates a pointer */
int
vm_access(struct vm_space *space, const void *buf, size_t nbyte, int prot)
{
    int i;
    int start_page;
    size_t required_pages;
    uint8_t *bitmap;
    struct va_map *vamap = NULL;

    if (VM_IS_KERN(prot) && VA_BOUNDCHECK(space->kva_map, buf, nbyte)) {
        vamap = space->kva_map;
    } else if (VA_BOUNDCHECK(space->uva_map, buf, nbyte)) {
        vamap = space->uva_map;
    }

    if (!vamap) {
        return -1;
    }

    start_page = PAGE_INDEX((uintptr_t)buf - vamap->base);
    required_pages = PAGE_COUNT(nbyte);

    bitmap = vamap->bitmap;

    for (i = 0; i < required_pages; i++) {
        if (!BITMAP_GET(bitmap, start_page + i)) {
            return -1;
        }
    }

    return 0;
}

/* finds a page from a given virtual address */
struct vm_block *
vm_find_block(struct vm_space *space, uintptr_t vaddr)
{
    list_iter_t iter;

    struct vm_block *block;
    struct vm_block *res;

    list_get_iter(&space->map, &iter);

    res = NULL;

    while (iter_move_next(&iter, (void**)&block)) {
        if (block->start_virtual == vaddr) {
            res = block;
            break;
        }
    }

    iter_close(&iter);

    return res;
}
