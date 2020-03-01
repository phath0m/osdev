#include <sys/bus.h>

void
shutdown()
{
    bus_interrupts_off();
    asm volatile("hlt");
}
