/*
 * clone.c - Clone implementation
 *
 * Allows for more granular creation of child processes
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
#include <machine/reg.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/interrupt.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/vm.h>

struct clone_state {
    struct proc *   proc;
    void *          func;
    void *          arg;
    uintptr_t       u_stack_top;
    uintptr_t       u_stack_bottom;
};

static int
init_child_proc(void *statep)
{
    /* defined in sys/i686/kern/sched.c */
    extern struct thread *sched_curr_thread;

    /* defined in sys/i686/kern/sched.c */
    extern struct thread *sched_curr_thread;

    /* defined in sys/i686/kern/usermode.asm */
    extern void return_to_usermode(uintptr_t target, uintptr_t stack, uintptr_t bp, uintptr_t ret);

    uint32_t *stack;
    struct clone_state *state;
    
    state = statep;

    sched_curr_thread->proc = state->proc;
    sched_curr_thread->u_stack_bottom = state->u_stack_bottom;
    sched_curr_thread->u_stack_top = state->u_stack_top;
    sched_curr_thread->tid = proc_get_new_pid();

    current_proc = state->proc;

    list_append(&current_proc->threads, sched_curr_thread);

    stack = (uint32_t*)state->u_stack_top;

    --stack;

    *(--stack) = (uintptr_t)state->arg;
    *(--stack) = NULL;

    free(statep);

    return_to_usermode((uintptr_t)state->func, (uintptr_t)stack, (uintptr_t)stack, 0);
    
    return 0;
}

int
proc_clone(void *func, void *stack, int flags, void *arg)
{
    extern struct vm_space *sched_curr_address_space;

    struct clone_state *state;

    if (flags != (CLONE_VM | CLONE_FILES)) {
        return -(ENOTSUP);
    }

    bus_interrupts_off();

    state = calloc(1, sizeof(struct clone_state));
    
    state->proc = current_proc;
    state->func = func;
    state->arg = arg;
    state->u_stack_top = (uintptr_t)stack;
    state->u_stack_bottom = (uintptr_t)stack - 65535;

    thread_run(init_child_proc, sched_curr_address_space, (void*)state);

    thread_yield();

    return current_proc->pid;
}
