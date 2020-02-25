#ifndef _SYS_VM_H
#define _SYS_VM_H

#include <ds/list.h>
#include <sys/types.h>

#define PROT_KERN   0x08
#define PROT_READ   0x04
#define PROT_WRITE  0x02
#define PROT_EXEC   0x01

#define VM_IS_WRITABLE(prot) ((prot & PROT_WRITE) != 0)
#define VM_IS_USER(prot) ((prot & PROT_KERN) == 0)

/* vamap_t is a bitmap used to track allocation of virtual addresses */
typedef uint8_t *   vamap_t;

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


struct va_map *va_map_new(uintptr_t base, uintptr_t limit);
uintptr_t va_alloc_block(struct va_map *vamap, uintptr_t addr, size_t length);
uintptr_t va_find_block(struct va_map *vamap, size_t length);
void va_mark_block(struct va_map *vamap, uintptr_t addr, size_t length);
void va_free_block(struct va_map *vamap, uintptr_t addr, size_t length);

struct vm_block *vm_find_block(struct vm_space *space, uintptr_t vaddr);


void *vm_map(struct vm_space *space, void *addr, size_t length, int prot);
void *vm_map_physical(struct vm_space *space, void *addr, uintptr_t physical, size_t length, int prot);
void *vm_share(struct vm_space *space1, struct vm_space *space2, void *addr1, void *addr2, size_t length, int prot);
void vm_clone(struct vm_space *space1, struct vm_space *space2);
void vm_space_destroy(struct vm_space *space);
struct vm_space *vm_space_new();
void vm_unmap(struct vm_space *space, void *addr, size_t length);

#endif
