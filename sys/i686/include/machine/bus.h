#ifndef _MACHINE_BUS_H
#define _MACHINE_BUS_H


static inline void
bus_interrupts_off()
{
    asm volatile("cli");
}

static inline void
bus_interrupts_on()
{
    asm volatile("sti");
}

#endif
