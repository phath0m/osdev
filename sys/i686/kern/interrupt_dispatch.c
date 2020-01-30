#include <sys/interrupt.h>
#include <sys/proc.h>
#include <sys/i686/interrupt.h>
#include <sys/i686/portio.h>

void
dispatch_intr(struct regs *regs)
{
    extern struct thread * sched_curr_thread;

    if (sched_curr_thread) {
        __sync_lock_test_and_set(&sched_curr_thread->interrupt_in_progress, 1);

        thread_interrupt_enter(sched_curr_thread, regs);
    }

    // defined in sys/interrupt.c
    extern void dispatch_to_intr_handler(int inum, struct regs *regs);
    
    uint8_t inum = regs->inum;

    if (inum >= 40) {
        io_write_byte(0xA0, 0x20);
    }

    dispatch_to_intr_handler(inum, regs);

    if (sched_curr_thread) {
        thread_interrupt_leave(sched_curr_thread, regs);
        
        __sync_lock_test_and_set(&sched_curr_thread->interrupt_in_progress, 0);
    }

    io_write_byte(0x20, 0x20);
}
