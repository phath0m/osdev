#include <machine/multiboot.h>
#include <machine/vm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/dev/textscreen.h>

// remove
#include <ds/list.h>
#include <sys/vm.h>
#include <sys/proc.h>

void *start_initramfs;
multiboot_info_t *multiboot_header;

int
run_kernel(void *state)
{
    /* defined in sys/i686/kern/sched.c */
    extern struct thread *sched_curr_thread;

    struct proc *init = proc_new();
    struct session *init_session = session_new(init);        
    struct pgrp *init_group = pgrp_new(init, init_session);

    init->thread = sched_curr_thread;
    init->group = init_group;

    sched_curr_thread->proc = init;
    current_proc = init;
    
    const char *args = (const char *)PTOKVA(multiboot_header->cmdline);

    if (kmain(args) != 0) {
        printf("Unable to boot kernel!\n");
    }

    printf("You may now shutdown your computer");
    
    asm volatile("cli");
    asm volatile("hlt");

    return 0;
}

void
_preinit(multiboot_info_t *multiboot_hdr)
{
    extern void _init();
    extern struct device serial0_device;

    uint32_t initrd = PTOKVA(*(uint32_t*)PTOKVA(multiboot_hdr->mods_addr));
    uint32_t heap = PTOKVA(*(uint32_t*)PTOKVA(multiboot_hdr->mods_addr + 4));

    asm volatile("cli");

    start_initramfs = (void*)(initrd);
    multiboot_header = multiboot_hdr;

    brk((void*)(heap));

    set_kernel_output(&serial0_device);

    printf("base   = 0x%p\n", KERNEL_VIRTUAL_BASE);
    printf("initrd = 0x%p\n", initrd);
    printf("heap   = 0x%p\n", heap);
    printf("usable memory map:\n"); 
    uint32_t usable_memory = 0;

    for (int i = 0; i < multiboot_hdr->mmap_length; i+= sizeof(struct multiboot_mmap_entry)) {

        struct multiboot_mmap_entry * entry = (struct multiboot_mmap_entry*)PTOKVA(multiboot_hdr->mmap_addr + i);

        if (entry->type == 1) {
            printf("  %p-%p\n", (int)entry->addr, (int)(entry->addr+entry->len));
            usable_memory += entry->len;
        }
    }

    printf("kernel: detected %dMB of usable memory\n", (usable_memory / 1024 / 1024));

    _init(); 

    thread_run((kthread_entry_t)run_kernel, NULL, NULL);

    /*
     * At this point, we are still using the stack initialized by the bootloader. 
     * We need to create a new thread using our own stack initialized at 0xFFFFFFFF
     *
     * Once the PIT fires, execution will switch to run_kernel which should be using
     * the new stack.
     *
     * Here, we just need to wait for it to fire before surrendering execution
     */

    asm volatile("sti");    
    
    for (;;) {
        asm volatile("hlt");
    }
}
