#ifndef SYS_INTERRUPT_H
#define SYS_INTERRUPT_H

struct regs;

typedef int (*intr_handler_t)(int inum, struct regs *regs);

int register_intr_handler(int inum, intr_handler_t handler);

#endif
