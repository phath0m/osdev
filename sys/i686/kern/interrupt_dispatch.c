#include <sys/interrupt.h>
#include <sys/i686/interrupt.h>
#include <sys/i686/portio.h>

void
dispatch_intr(struct regs *regs)
{
    // defined in sys/interrupt.c
    extern void dispatch_to_intr_handler(int inum, struct regs *regs);
    
    uint8_t inum = regs->inum;

    if (inum >= 40) {
        io_write_byte(0xA0, 0x20);
    }

    dispatch_to_intr_handler(inum, regs);

    io_write_byte(0x20, 0x20);
}
