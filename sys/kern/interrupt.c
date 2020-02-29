#include <sys/bus.h>

intr_handler_t intr_handlers[256];

void
dispatch_to_intr_handler(int inum, struct regs *regs)
{
    intr_handler_t handler = intr_handlers[inum];

    if (handler) {
        handler(inum, regs);
    }
}

int
register_intr_handler(int inum, intr_handler_t handler)
{
    intr_handlers[inum] = handler;
    
    return 0;
}
