#include <rtl/malloc.h>
#include <rtl/printf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/dev/textscreen.h>
#include <sys/i686/multiboot.h>

// remove
#include <rtl/list.h>
#include <rtl/string.h>
#include <sys/vm.h>
#include <sys/proc.h>

#define KERNEL_BASE 0xC0000000

void *start_initramfs;

int
run_kernel(void *state)
{
    if (kmain() != 0) {
        printf("Unable to boot kernel!\n");
    }

    printf("You may now shutdown your computer");

    asm volatile("cli");
    asm volatile("hlt");

    return 0;
}

int
counter1(void *state)
{
    for (int i = 0; ; i += 2) {
        printf("counter1 is at %d\n", i);
    }
    return 0;
}

int
counter2(void*state)
{
    for (int i = 1; ; i += 2) {
        printf("counter2 is at %d\n", i);
    }
    return 0;
}

void
_preinit(multiboot_info_t *multiboot_hdr)
{
    extern void _init();
    extern struct device textscreen_device;

    uint32_t initrd = KERNEL_BASE + *(uint32_t*)(KERNEL_BASE + multiboot_hdr->mods_addr);
    uint32_t heap = KERNEL_BASE + *(uint32_t*)(KERNEL_BASE + multiboot_hdr->mods_addr + 4);

    asm volatile("cli");

    start_initramfs = (void*)(initrd);

    brk((void*)(heap));
    
    device_ioctl(&textscreen_device, TEXTSCREEN_SETFG, 0x0F);

    kset_output(&textscreen_device);
    
    _init();

    uint32_t usable_memory = 0;

    for (int i = 0; i < multiboot_hdr->mmap_length; i+= sizeof(struct multiboot_mmap_entry)) {

        struct multiboot_mmap_entry * entry = (struct multiboot_mmap_entry*)(KERNEL_BASE + multiboot_hdr->mmap_addr + i);

        uint32_t start = entry->addr;
        uint32_t end = entry->addr + entry->len;
        printf("memory region %p-%p\n", start, end);
        
        if (entry->type == 1) {
            usable_memory += entry->len;
        }
    }
    printf("detected %dMB of usable memory\n", (usable_memory / 1024 / 1024));
    printf("heap is located at %p\n", heap);

    sched_run_kthread((kthread_entry_t)counter2);
    sched_run_kthread((kthread_entry_t)counter1);
    

    //sched_run_kthread((kthread_entry_t)counter1);
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
