#include <machine/reg.h>
#include <sys/errno.h>
#include <sys/file.h>
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
    struct clone_state *state = (struct clone_state*)statep;

    /* defined in sys/i686/kern/sched.c */
    extern struct thread *sched_curr_thread;

    sched_curr_thread->proc = state->proc;
    sched_curr_thread->u_stack_bottom = state->u_stack_bottom;
    sched_curr_thread->u_stack_top = state->u_stack_top;
    sched_curr_thread->tid = proc_get_new_pid();

    current_proc = state->proc;

    list_append(&current_proc->threads, sched_curr_thread);

    free(statep);

    uint32_t *stack = (uint32_t*)state->u_stack_top;

    --stack;

    *(--stack) = (uintptr_t)state->arg;
    *(--stack) = NULL;

    /* defined in sys/i686/kern/usermode.asm */
    extern void return_to_usermode(uintptr_t target, uintptr_t stack, uintptr_t bp, uintptr_t ret);

    return_to_usermode((uintptr_t)state->func, (uintptr_t)stack, (uintptr_t)stack, 0);
    
    return 0;
}

int
proc_clone(void *func, void *stack, int flags, void *arg)
{
    if (flags != (CLONE_VM | CLONE_FILES)) {
        return -(ENOTSUP);
    }

    asm volatile("cli");

    extern struct vm_space *sched_curr_address_space;

    struct clone_state *state = (struct clone_state*)calloc(1, sizeof(struct clone_state));
    
    state->proc = current_proc;
    state->func = func;
    state->arg = arg;
    state->u_stack_top = (uintptr_t)stack;
    state->u_stack_bottom = (uintptr_t)stack - 65535;

    thread_run(init_child_proc, sched_curr_address_space, (void*)state);

    thread_yield();

    return current_proc->pid;
}
