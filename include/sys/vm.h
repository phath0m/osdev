#ifndef SYS_MM_H
#define SYS_MM_H

#include <rtl/list.h>
#include <rtl/types.h>

#define PROT_KERN   0x08
#define PROT_READ   0x04
#define PROT_WRITE  0x02
#define PROT_EXEC   0x01

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
    uintptr_t   user_brk;
    uintptr_t   kernel_brk;
    uintptr_t   stack;
    /* used for the architecture specific virtual memory implementation */
    void *      state_physical;
    void *      state_virtual;
};

void *vm_map(struct vm_space *space, void *addr, size_t length, int prot);
void *vm_share(struct vm_space *space1, struct vm_space *space2, void *addr1, void *addr2, size_t length, int prot);
void vm_space_destroy(struct vm_space *space);
struct vm_space *vm_space_new();
void vm_unmap(struct vm_space *space, void *addr, size_t length);

#endif
