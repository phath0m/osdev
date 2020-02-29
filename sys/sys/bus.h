#ifndef _SYS_BUS_H
#define _SYS_BUS_H

#include <machine/bus.h>

struct regs;

typedef int (*intr_handler_t)(int inum, struct regs *regs);

int bus_register_intr(int inum, intr_handler_t handler);

#endif
