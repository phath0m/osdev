#include <rtl/malloc.h>
#include <rtl/string.h>
#include <rtl/types.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/i686/interrupt.h>
// remove
#include <rtl/printf.h>

struct fork_state {
    struct proc *   proc;
    struct regs     regs;
};

static void
copy_image(struct proc *proc, struct vm_space *new_space)
{
    size_t image_size = ((proc->brk - proc->base) + 0x1000) & 0xFFFFF000;
    
    vm_map(new_space, (void*)proc->base, image_size, PROT_READ | PROT_WRITE);

    void *image = vm_share(proc->thread->address_space, new_space, NULL, (void*)proc->base,
                           image_size, PROT_READ | PROT_WRITE | PROT_KERN);

    memcpy(image, (void*)proc->base, image_size);

    vm_unmap(proc->thread->address_space, image, image_size);
}

static void
copy_fildes(struct proc *proc, struct proc *new_proc)
{
    for (int i = 0; i < 10; i++) {
        new_proc->files[i] = proc->files[i];
    }
}

static void
copy_stack(struct thread *thread, struct vm_space *new_space)
{
    size_t stack_size = thread->u_stack_top - thread->u_stack_bottom;

    vm_map(new_space, (void*)thread->u_stack_bottom, stack_size, PROT_READ | PROT_WRITE);

    void *stack = vm_share(thread->address_space, new_space, NULL, (void*)thread->u_stack_bottom,
                           stack_size, PROT_READ | PROT_WRITE | PROT_KERN);
    memcpy(stack, (void*)thread->u_stack_bottom, stack_size);

    vm_unmap(thread->address_space, stack, stack_size);
}

static int
init_child_proc(void *statep)
{
    struct fork_state *state = (struct fork_state*)statep;
    struct proc *proc = state->proc;

    /* defined in sys/i686/kern/sched.c */
    extern struct thread *sched_curr_thread;

    proc->thread = sched_curr_thread;
    sched_curr_thread->proc = state->proc;

    /* defined in sys/i686/kern/usermode.asm */
    extern void return_to_usermode(uintptr_t target, uintptr_t stack);

    return_to_usermode(state->regs.eip, state->regs.uesp);

    return 0;
}

int
proc_fork(struct regs *regs)
{
    struct proc *proc = current_proc;
    struct proc *new_proc = proc_new();
    struct vm_space *new_space = vm_space_new();

    new_proc->base = proc->base;
    new_proc->brk = proc->brk;
    new_proc->root = proc->root;

    copy_stack(proc->thread, new_space);
    copy_image(proc, new_space);
    copy_fildes(proc, new_proc);

    struct fork_state *state = (struct fork_state*)calloc(0, sizeof(struct fork_state));
    state->proc = new_proc;

    memcpy(&state->regs, regs, sizeof(struct regs));

    sched_run_kthread(init_child_proc, new_space, (void*)state);

    return 0;
}
