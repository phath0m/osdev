#include <rtl/malloc.h>
#include <rtl/printf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/dev/textscreen.h>
#include <sys/i686/multiboot.h>

void
_preinit(multiboot_info_t *multiboot_hdr)
{
    extern uintptr_t end_bss;

    extern void _init();

    extern struct device textscreen_device;

    brk((void*)end_bss);

    device_ioctl(&textscreen_device, TEXTSCREEN_SETFG, 0x0F);

    kset_output(&textscreen_device);

    _init();

    if (kmain() != 0) {
        printf("Unable to boot kernel!\n");
    }

    printf("You may now shutdown your computer");

    asm volatile("cli");
    asm volatile("hlt");
}
