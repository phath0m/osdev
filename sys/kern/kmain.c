#include <rtl/printf.h>
#include <sys/kernel.h>

int
kmain()
{
    printf("Hello, World!\n");

    /*
     * There isn't much we can do right now :P
     */
    asm volatile("cli");
    asm volatile("hlt");

    return 0;
}
