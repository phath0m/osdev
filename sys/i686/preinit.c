#include <rtl/malloc.h>
#include <rtl/printf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/dev/textscreen.h>
#include <sys/i686/multiboot.h>

void *start_initramfs;

void
_preinit(multiboot_info_t *multiboot_hdr)
{
    extern uint32_t heap_start;

    extern void _init();

    extern struct device textscreen_device;

    uint32_t initrd = *(uint32_t*)(multiboot_hdr->mods_addr);
    uint32_t heap = *(uint32_t*)(multiboot_hdr->mods_addr + 4);

    start_initramfs = (void*)initrd;

    brk((void*)heap);

    device_ioctl(&textscreen_device, TEXTSCREEN_SETFG, 0x0F);

    kset_output(&textscreen_device);

    _init();
    printf("initrd is at: %x\n", multiboot_hdr->mods_addr);
    printf("kernel heap is at: %x\n", &heap_start);
    printf("test: %s\n", (char*)initrd);

    if (kmain() != 0) {
        printf("Unable to boot kernel!\n");
    }

    printf("You may now shutdown your computer");

    asm volatile("cli");
    asm volatile("hlt");
}
