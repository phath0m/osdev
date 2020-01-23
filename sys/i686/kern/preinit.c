#include <stdio.h>
#include <stdlib.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/dev/textscreen.h>
#include <sys/i686/multiboot.h>

// remove
#include <ds/list.h>
#include <sys/vm.h>
#include <sys/proc.h>
#include <sys/thread.h>

#define KERNEL_BASE 0xC0000000

void *start_initramfs;
multiboot_info_t *multiboot_header;

int
run_kernel(void *state)
{
    /* defined in sys/i686/kern/sched.c */
    extern struct thread *sched_curr_thread;

    struct proc *init = proc_new();
        
    init->thread = sched_curr_thread;
    init->group = calloc(1, sizeof(struct pgrp));

    sched_curr_thread->proc = init;
    current_proc = init;
    
    if (kmain() != 0) {
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

    uint32_t initrd = KERNEL_BASE + *(uint32_t*)(KERNEL_BASE + multiboot_hdr->mods_addr);
    uint32_t heap = KERNEL_BASE + *(uint32_t*)(KERNEL_BASE + multiboot_hdr->mods_addr + 4);

    asm volatile("cli");

    start_initramfs = (void*)(initrd);
    multiboot_header = multiboot_hdr;

    brk((void*)(heap));
    
    //device_ioctl(&textscreen_device, TEXTSCREEN_SETFG, 0x0F);

    kset_output(&serial0_device);

    printf("base   = 0x%p\n", KERNEL_BASE);
    printf("initrd = 0x%p\n", initrd);
    printf("heap   = 0x%p\n", heap);
    
    _init();

    uint32_t usable_memory = 0;

    for (int i = 0; i < multiboot_hdr->mmap_length; i+= sizeof(struct multiboot_mmap_entry)) {

        struct multiboot_mmap_entry * entry = (struct multiboot_mmap_entry*)(KERNEL_BASE + multiboot_hdr->mmap_addr + i);

        if (entry->type == 1) {
            usable_memory += entry->len;
        }
    }
    printf("kernel: detected %dMB of usable memory\n", (usable_memory / 1024 / 1024));
    
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
