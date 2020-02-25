#ifndef _MACHINE_VM_H
#define _MACHINE_VM_H

#include <sys/types.h>

struct page_directory_entry {
    bool        present : 1;
    bool        read_write : 1;
    bool        user : 1;
    bool        write_through : 1;
    bool        cache : 1;
    bool        accessed : 1;
    bool        zero : 1;
    bool        size : 1;
    uint8_t     unused : 4;
    uint32_t    address : 20; 
} __attribute__((packed));

struct page_entry {
    bool        present : 1;
    bool        read_write : 1;
    bool        user : 1;
    bool        accessed : 1;
    bool        dirty : 1;
    uint32_t    unused : 7;
    uint32_t    frame : 20;
} __attribute__((packed));

struct page_table {
    struct page_entry    pages[1024];
};

struct page_directory {
    struct page_directory_entry   tables[1024];
};

#define KERNEL_VIRTUAL_BASE 0xC0000000
#define PAGE_SIZE           0x1000

#define ALIGN_ADDRESS(addr) (((uintptr_t)addr) & 0xFFFFF000)
#define PAGE_COUNT(amount) (((amount - 1) >> 12) + 1)
#define PAGE_OFFSET(addr) (addr >> 12)

#endif
