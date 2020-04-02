/*
 * sched.c - scheduler implementation
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
#include <ds/list.h>
#include <machine/portio.h>
#include <machine/reg.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/timer.h>
#include <sys/vm.h>

// list of processes to run
struct list         run_queue;
struct list         dead_threads;

struct proc *       current_proc;
struct vm_space *   sched_curr_address_space;
struct thread *     sched_curr_thread;

uintptr_t           sched_curr_page_dir;

uint32_t            sched_ticks     = 0;
uint32_t            sched_hz        = 1000;

static void
sched_reap_threads()
{
    list_iter_t iter;
    list_get_iter(&dead_threads, &iter);

    struct thread *thread;
    while (iter_move_next(&iter, (void**)&thread)) {
        thread_destroy(thread);
    }

    iter_close(&iter);
    list_destroy(&dead_threads, false);
}

int
sched_get_next_proc(uintptr_t prev_esp)
{
    /* defined in sys/i686/kern/interrupt.c */
    extern void set_tss_esp0(uint32_t esp0);

    timer_tick();

    sched_ticks++;

    struct thread *next_thread = NULL;

    if (list_remove_front(&run_queue, (void**)&next_thread) && next_thread != sched_curr_thread) {
        
        if (sched_curr_thread) {
            sched_curr_thread->stack = prev_esp;

            if (sched_curr_thread->state == SRUN) {
                
                if (LIST_SIZE(&dead_threads) > 0) {
                    sched_reap_threads();
                }

                list_append(&run_queue, sched_curr_thread);
            }
        }

        sched_curr_address_space = next_thread->address_space;
        sched_curr_thread = next_thread;
        sched_curr_page_dir = (uintptr_t)next_thread->address_space->state_physical;

        current_proc = next_thread->proc;

        set_tss_esp0(next_thread->stack_top);

        return next_thread->stack;
    }

    if (sched_curr_thread) {
        current_proc = sched_curr_thread->proc;
        set_tss_esp0(sched_curr_thread->stack_top);
    }

    return prev_esp;
}

void
thread_interrupt_enter(struct thread *thread, struct regs *regs)
{
}

void
thread_interrupt_leave(struct thread *thread, struct regs *regs)
{ 
    if (thread->terminated) {
        thread_schedule(SDEAD, thread);
       
        bus_interrupts_on(); 

        for (;;) {
            thread_yield();
        }
    }

    /* reset this flag, we've exited kernel space */
    thread->exit_requested = 0;

    if (LIST_SIZE(&thread->pending_signals) > 0 && regs->eip < 0xC0000000) {
        struct sigcontext *ctx;

        list_remove_back(&thread->pending_signals, (void**)&ctx);

        ctx->regs = calloc(1, sizeof(struct regs));

        memcpy(ctx->regs, regs, sizeof(struct regs));

        regs->eip = ctx->handler_func;
        regs->edx = ctx->arg;
        regs->ecx = ctx->signo;

        list_append(&thread->signal_stack, ctx);
    }
}

void
thread_run(kthread_entry_t entrypoint, struct vm_space *space, void *arg)
{
    struct thread *thread = thread_new(space);

    if (space) {
        thread->address_space = space;
    } else {
        thread->address_space = vm_space_new();
    }

    thread->state = SRUN;

    uint32_t *stack_base = calloc(1, 65536);
    uint32_t *stack_top = &stack_base[16380];
    uint32_t *stack = (uint32_t*)stack_top;
    
    *--stack = (uint32_t)arg;
    *--stack = (uintptr_t)stack_top; /* idk (I think ESP?)) */
    *--stack = 0x202; /* EFLAGS???? */
    *--stack = 0x08; /* code segment */
    *--stack = (uint32_t)entrypoint;
    *--stack = 0;       /* EAX */
    *--stack = 0;       /* ECX */
    *--stack = 0;       /* EDX */
    *--stack = 0;       /* EBX */
    *--stack = 0;       /* reserved */
    *--stack = 0;       /* EBP */
    *--stack = 0;       /* ESI */
    *--stack = 0;       /* EDI */
    *--stack = 0x10;    /* GS */
    *--stack = 0x10;    /* ES */
    *--stack = 0x10;    /* FS */
    *--stack = 0x10;    /* GS  */

    thread->stack = (uintptr_t)stack;
    thread->stack_top = (uintptr_t)stack_top;
    thread->stack_base = (uintptr_t)stack_base;

    list_append(&run_queue, thread);
}

void
thread_yield()
{
    bus_interrupts_on();
    asm volatile("hlt");
}

void
thread_schedule(int state, struct thread *thread)
{
    if (thread->state == state) {
        return;
    }

    thread->state = state;

    switch (state) {
        case SRUN:
            list_append(&run_queue, thread);
            break;
        case SDEAD:
            list_append(&dead_threads, thread);
            break;
    }
}

void
sched_init()
{
    uint32_t divisor = 1193180 / sched_hz;
    io_write_byte(0x43, 0x36);
    io_write_byte(0x40, divisor & 0xFF);
    io_write_byte(0x40, (divisor >> 8) & 0xFF);

    sched_curr_address_space = vm_space_new();

    memset(&run_queue, 0, sizeof(struct list));

    asm volatile("mov %0, %%cr3" :: "r"((uint32_t)sched_curr_address_space->state_physical));
    asm volatile("mov %cr3, %eax; mov %eax, %cr3;");
}
