#ifndef _MACHINE_BUS_H
#define _MACHINE_BUS_H

#define DEVICE_PCI  0x01    /* identifies that device is attached to PCI bus */

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
