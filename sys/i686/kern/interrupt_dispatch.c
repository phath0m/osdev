/*
 * interrupt_dispatch.c - Dispatch interrupts to their registered handler
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <machine/portio.h>
#include <machine/reg.h>
#include <sys/interrupt.h>
#include <sys/proc.h>

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
    
    io_write_byte(0x20, 0x20);

    if (sched_curr_thread) {
        thread_interrupt_leave(sched_curr_thread, regs);
        
        __sync_lock_test_and_set(&sched_curr_thread->interrupt_in_progress, 0);
    }
}
