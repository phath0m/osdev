#ifndef _SYS_VM_H
#define _SYS_VM_H

#include <ds/list.h>
#include <sys/types.h>

#define VM_KERN   0x08
#define VM_READ   0x04
#define VM_WRITE  0x02
#define VM_EXEC   0x01

#define VM_IS_WRITABLE(prot)    ((prot & VM_WRITE) != 0)
#define VM_IS_USER(prot)        ((prot & VM_KERN) == 0)
#define VM_IS_KERN(prot)        ((prot & VM_KERN) == VM_KERN)

/* structure used to track allocation of virtual addresses */
struct va_map {
    uintptr_t   base; /* starting virtual address*/
    uintptr_t   limit; /* ending virtual address */
    uint8_t     bitmap[]; /* bitmap used to actually track allocations */
};

struct vm_block {
    size_t      size;
    uintptr_t   start_physical;
    uintptr_t   start_virtual;
    int         prot;
    /* this is used internally by native architecture for reference counting and shouldn't be touched */
    void *      state;
};

struct vm_space {
    struct list map;
    uintptr_t   kernel_brk;
    uintptr_t   kernel_end;
    uintptr_t   stack;
    struct va_map * uva_map; /* usermode virtual address map */
    struct va_map * kva_map; /* kernel mode virtual address map*/

    /* used for the architecture specific virtual memory implementation */
    void *      state_physical;
    void *      state_virtual;
};

/*
 * various metrics I started tracking for debugging purposes. The bugs I was
 * originally trying to fix have been resolved, however, I thought I'd keep
 * this around
 */
struct vm_statistics {
    uint32_t    frame_count; /* how many total frames */
    uint32_t    page_count; /* how many total page objects */
    uint32_t    page_table_count; /* how many page tables */
    uint32_t    vmspace_count;  /* how many address spaces*/
};

extern struct vm_statistics vm_stat;

#define VMSTAT_DEC_FRAME_COUNT(v) ((v)->frame_count--)
#define VMSTAT_DEC_PAGE_COUNT(v) ((v)->page_count--)
#define VMSTAT_DEC_PAGE_TABLE_COUNT(v) ((v)->page_table_count--)
#define VMSTAT_DEC_VM_SPACE_COUNT(v) ((v)->vmspace_count--)

#define VMSTAT_INC_FRAME_COUNT(v) ((v)->frame_count++)
#define VMSTAT_INC_PAGE_COUNT(v) ((v)->page_count++)
#define VMSTAT_INC_PAGE_TABLE_COUNT(v) ((v)->page_table_count++)
#define VMSTAT_INC_VM_SPACE_COUNT(v) ((v)->vmspace_count++)

#define VA_BOUNDCHECK(vamap, addr, len) (((uintptr_t)(addr) >= (vamap)->base) && \
                                         ((uintptr_t)(addr) + (uintptr_t)(len) <= (vamap)->limit))

struct va_map *     va_map_new(uintptr_t, uintptr_t);
uintptr_t           va_alloc_block(struct va_map *, uintptr_t, size_t);
uintptr_t           va_find_block(struct va_map *, size_t);
void                va_mark_block(struct va_map *, uintptr_t, size_t);
void                va_free_block(struct va_map *, uintptr_t, size_t);

int                 vm_access(struct vm_space *, const void *, size_t, int);

struct vm_block *   vm_find_block(struct vm_space *, uintptr_t);

void *              vm_map(struct vm_space *, void *, size_t, int);
void *              vm_map_physical(struct vm_space *, void *, uintptr_t, size_t, int);
void *              vm_share(struct vm_space *, struct vm_space *, void *, void *, size_t, int);
void                vm_clone(struct vm_space *, struct vm_space *);
void                vm_space_destroy(struct vm_space *);
struct              vm_space *vm_space_new();
void                vm_unmap(struct vm_space *, void *, size_t);

#endif
