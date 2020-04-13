#ifndef _MACHINE_VM_H
#define _MACHINE_VM_H

#include <sys/types.h>

#define KERNEL_VIRTUAL_BASE 0xC0000000
#define PAGE_SIZE           0x1000

#define ALIGN_ADDRESS(addr) (((uintptr_t)(addr)) & 0xFFFFF000)
#define PAGE_COUNT(amount) ((((amount) - 1) >> 12) + 1)
#define PAGE_INDEX(addr) ((addr) >> 12)
#define PAGE_OFFSET(addr) ((uintptr_t)(addr) & 0xFFF);
#define FRAME_INDEX(addr) PAGE_INDEX(addr)

/* physical address to kernel virtual address */
#define PTOKVA(addr) ((uintptr_t)KERNEL_VIRTUAL_BASE+(uintptr_t)(addr))
/* higher half virtual address to physical address */
#define KVATOP(addr) (((uintptr_t)(addr)) - KERNEL_VIRTUAL_BASE)

#endif
