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
#include <sys/bus.h>
#include <sys/device.h>
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
static void
ksym_init()
{
    struct elf32_shdr *shdr = (struct elf32_shdr*)PTOKVA(multiboot_header->u.elf_sec.addr);
    
    struct elf32_shdr *symtab = &shdr[15];
    struct elf32_shdr *strtab = &shdr[16];
    
    char *all_strings = (char*)PTOKVA(strtab->sh_addr);
    struct elf32_sym *all_syms = (struct elf32_sym*)PTOKVA(symtab->sh_addr);
    
    int nents = symtab->sh_size / sizeof(struct elf32_sym);
    
    for (int i = 0; i < nents; i++) {
        struct elf32_sym *sym = &all_syms[i];
        char *name = &all_strings[sym->st_name];

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

    uint32_t initrd = PTOKVA(*(uint32_t*)PTOKVA(multiboot_hdr->mods_addr));
    uint32_t heap = PTOKVA(*(uint32_t*)PTOKVA(multiboot_hdr->mods_addr + 4));

    bus_interrupts_off();

    start_initramfs = (void*)(initrd);
    multiboot_header = multiboot_hdr;

    brk((void*)(heap));

    /* set the initial console */
    set_kernel_output(&serial0_device);

    /* load kernel symbols */
    ksym_init();

    printf("base=0x%p initrd=0x%p heap=0x%p\n", KERNEL_VIRTUAL_BASE, initrd, heap);
    printf("physical memory map:\n"); 

    uint32_t usable_memory = 0;

    for (int i = 0; i < multiboot_hdr->mmap_length; i+= sizeof(struct multiboot_mmap_entry)) {
        struct multiboot_mmap_entry * entry = (struct multiboot_mmap_entry*)PTOKVA(multiboot_hdr->mmap_addr + i);

        if (entry->type == 1) {
            printf("  %p-%p\n", (int)entry->addr, (int)(entry->addr+entry->len));
            usable_memory += entry->len;
        }
    }

    printf("kernel: detected %dMB of usable memory\n", (usable_memory / 1024 / 1024));

    extern void vm_init();
    extern void intr_init();
    extern void syscall_init();
    extern void traps_init();
    extern void sched_init();

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

    /* move this somewhere else*/
    extern void machine_dev_init();

    machine_dev_init();

    const char *args = (const char *)PTOKVA(multiboot_header->cmdline);
    
    if (kmain(args) != 0) {
        printf("Unable to boot kernel!\n");
    }

    for (;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}
