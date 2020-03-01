/*
 * architecture independent virtual memory code
 *
 * Really, all this file is responsible for is controlling allocation of virtual
 * address. Everything else of note is currently in the architecture dependent
 * memory manager (sys/<ARCH>/kern/vm.c)
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
    size_t bitmap_size = PAGE_COUNT(limit - base);

    struct va_map *vamap = calloc(1, sizeof(struct va_map) + bitmap_size);

    vamap->base = base;
    vamap->limit = limit;

    return vamap;
}

/* finds a suitable virtual address */
uintptr_t
va_find_block(struct va_map *vamap, size_t length)
{
    size_t required_pages = PAGE_COUNT(length);
    size_t max_pages = PAGE_COUNT(vamap->limit);

    int free_page_count = 0;

    uint8_t *bitmap = vamap->bitmap;

    for (int i = 1; i < max_pages; i++) {
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
    if (addr + length > vamap->limit) {
        return;
    }

    addr -= vamap->base;

    size_t required_pages = PAGE_COUNT(length);

    int start_page = PAGE_INDEX(addr);

    uint8_t *bitmap = vamap->bitmap;

    for (int i = 0; i < required_pages; i++) {
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
    if (addr + length > vamap->limit) {
        return;
    }

    addr -= vamap->base;

    size_t required_pages = PAGE_COUNT(length);
    int start_page = PAGE_INDEX(addr);

    uint8_t *bitmap = vamap->bitmap;

    for (int i = 0; i < required_pages; i++) {
        BITMAP_CLEAR(bitmap, start_page + i);
    }
}

/* validates a pointer */
int
vm_access(struct vm_space *space, const void *buf, size_t nbyte, int prot)
{
    struct va_map *vamap = NULL;

    if (VM_IS_KERN(prot) && VA_BOUNDCHECK(space->kva_map, buf, nbyte)) {
        vamap = space->kva_map;
    } else if (VA_BOUNDCHECK(space->uva_map, buf, nbyte)) {
        vamap = space->uva_map;
    }

    if (!vamap) {
        return -1;
    }

    int start_page = PAGE_INDEX((uintptr_t)buf - vamap->base);
    size_t required_pages = PAGE_COUNT(nbyte);

    uint8_t *bitmap = vamap->bitmap;

    for (int i = 0; i < required_pages; i++) {
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

    list_get_iter(&space->map, &iter);

    struct vm_block *block;
    struct vm_block *res = NULL;

    while (iter_move_next(&iter, (void**)&block)) {
        if (block->start_virtual == vaddr) {
            res = block;
            break;
        }
    }

    iter_close(&iter);

    return res;
}
