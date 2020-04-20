#ifndef _MACHINE_BUS_H
#define _MACHINE_BUS_H

#define DEVICE_PCI  0x01    /* identifies that device is attached to PCI bus */

extern int interrupts_enabled;

static inline void
bus_interrupts_off()
{
    interrupts_enabled = 0;
    asm volatile("cli");
}

static inline void
bus_interrupts_on()
{
    asm volatile("sti");
    interrupts_enabled = 1;
}

#endif
