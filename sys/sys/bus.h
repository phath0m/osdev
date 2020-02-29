#ifndef _SYS_BUS_H
#define _SYS_BUS_H

#include <machine/bus.h>

struct regs;

typedef int (*intr_handler_t)(int inum, struct regs *regs);

int register_intr_handler(int inum, intr_handler_t handler);

#endif
