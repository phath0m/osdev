/*
 * preinit.c - machine dependent kernel initialization
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

#include <machine/elf32.h>
#include <machine/multiboot.h>
#include <machine/vm.h>
#include <sys/cdev.h>
#include <sys/interrupt.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>

// remove
#include <ds/list.h>
#include <sys/vm.h>
#include <sys/proc.h>

void *start_initramfs;
multiboot_info_t *multiboot_header;

/* load all kernel symbols */
void
ksym_init()
{
    int i;
    int nents;

    char *all_strings;

    struct elf32_shdr *shdr;
    struct elf32_shdr *symtab;
    struct elf32_shdr *strtab;
    struct elf32_sym *all_syms;

    shdr = (struct elf32_shdr*)PTOKVA(multiboot_header->u.elf_sec.addr);

    /* note to self: this is a hack, we're assuming where these are*/
    symtab = &shdr[16];
    strtab = &shdr[17];
    
    all_strings = (char*)PTOKVA(strtab->sh_addr);
    all_syms = (struct elf32_sym*)PTOKVA(symtab->sh_addr);
    
    nents = symtab->sh_size / sizeof(struct elf32_sym);
    
    for (i = 0; i < nents; i++) {
        char *name;
        struct elf32_sym *sym;

        sym = &all_syms[i];
        name = &all_strings[sym->st_name];

        if (name) {
            ksym_declare(name, sym->st_value);
        }
    }

}

void
_preinit(multiboot_info_t *multiboot_hdr)
{
    extern void _init();
    extern struct cdev serial0_device;

    extern void machine_dev_init();
    extern void vm_init();
    extern void intr_init();
    extern void syscall_init();
    extern void traps_init();
    extern void sched_init();

    int i;
    uint32_t avail_memory;
    uint32_t initrd;
    uint32_t heap;
    uint32_t real_memory;

    const char *args;

    initrd = PTOKVA(*(uint32_t*)PTOKVA(multiboot_hdr->mods_addr));
    heap = PTOKVA(*(uint32_t*)PTOKVA(multiboot_hdr->mods_addr + 4));

    bus_interrupts_off();

    start_initramfs = (void*)(initrd);
    multiboot_header = multiboot_hdr;

    brk((void*)(heap));

    /* set the initial console */
    set_kernel_output(&serial0_device);

    /* load kernel symbols */
    ksym_init();

    printf(ELYSIUM_SYSNAME " " ELYSIUM_RELEASE " " ELYSIUM_VERSION "\n\r");
    printf("base=0x%p initrd=0x%p heap=0x%p\n\r", KERNEL_VIRTUAL_BASE, initrd, heap);
    printf("physical memory map:\n\r"); 

    real_memory = 0;
    avail_memory = 0;

    for (i = 0; i < multiboot_hdr->mmap_length; i+= sizeof(struct multiboot_mmap_entry)) {
        struct multiboot_mmap_entry * entry;
        
        entry = (struct multiboot_mmap_entry*)PTOKVA(multiboot_hdr->mmap_addr + i);

        if (entry->type == 1) {
            printf("  %p-%p\n\r", (int)entry->addr, (int)(entry->addr+entry->len));
            avail_memory += entry->len;
        }
        
        real_memory += entry->len;
    }

    printf("real  mem = %d KB\n\r", real_memory / 1024);
    printf("avail mem = %d KB\n\r", avail_memory / 1024);

    /* initialize virtual memory first*/
    vm_init();

    /* next comes interrupts */
    intr_init();

    /* now register our syscall dispatcher */
    syscall_init();

    /* now exception handlers */
    traps_init();

    /* finally, the scheduler */
    sched_init();

    /* I should move this somewhere else */
    machine_dev_init();

    args = (const char *)PTOKVA(multiboot_header->cmdline);
    
    if (kmain(args) != 0) {
        printf("Unable to boot kernel!\n");
    }

    for (;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}
