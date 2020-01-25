#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/vm.h>
#include <sys/i686/interrupt.h>

struct fork_state {
    struct proc *   proc;
    uintptr_t       u_stack_top;
    uintptr_t       u_stack_bottom;
    struct regs     regs;
};

static void
copy_image(struct proc *proc, struct vm_space *new_space)
{
    size_t image_size = ((proc->brk - proc->base));
    
    vm_map(new_space, (void*)proc->base, image_size, PROT_READ | PROT_WRITE);

    void *image = vm_share(proc->thread->address_space, new_space, NULL, (void*)proc->base,
                           image_size, PROT_READ | PROT_WRITE | PROT_KERN);

    memcpy(image, (void*)proc->base, image_size);

    vm_unmap(proc->thread->address_space, image, image_size);
}

static void
copy_fildes(struct proc *proc, struct proc *new_proc)
{
    for (int i = 0; i < 4096; i++) {
        if (proc->files[i]) {
            new_proc->files[i] = vfs_duplicate_file(proc->files[i]);
        }
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
    sched_curr_thread->u_stack_bottom = state->u_stack_bottom;
    sched_curr_thread->u_stack_top = state->u_stack_top;

    current_proc = proc;

    uintptr_t eip = state->regs.eip;
    uintptr_t esp = state->regs.uesp;
    uintptr_t ebp = state->regs.ebp;

    free(statep);

    /* defined in sys/i686/kern/usermode.asm */
    extern void return_to_usermode(uintptr_t target, uintptr_t stack, uintptr_t bp, uintptr_t ret);

    return_to_usermode(eip, esp, ebp, 0);
    
    return 0;
}

int
proc_fork(struct regs *regs)
{
    asm volatile("cli");

    struct proc *proc = current_proc;
    struct proc *new_proc = proc_new();
    struct vm_space *new_space = vm_space_new();

    strncpy(new_proc->name, proc->name, sizeof(new_proc->name));

    memcpy(&new_proc->creds, &proc->creds, sizeof(struct cred));

    new_proc->base = proc->base;
    new_proc->brk = proc->brk;
    new_proc->cwd = proc->cwd;
    new_proc->parent = proc;
    new_proc->root = proc->root;
    new_proc->umask = proc->umask;
    new_proc->group = proc->group;

    INC_NODE_REF(proc->root);
    INC_NODE_REF(proc->cwd);

    list_append(&proc->children, new_proc);

    KASSERT(proc->group != NULL, "process cannot inherit a NULL group");

    list_append(&proc->group->members, new_proc);

    copy_stack(proc->thread, new_space);
    copy_image(proc, new_space);
    copy_fildes(proc, new_proc);

    struct fork_state *state = (struct fork_state*)calloc(0, sizeof(struct fork_state));

    state->proc = new_proc;
    state->u_stack_top = proc->thread->u_stack_top;
    state->u_stack_bottom = proc->thread->u_stack_bottom;

    memcpy(&state->regs, regs, sizeof(struct regs));

    thread_run(init_child_proc, new_space, (void*)state);

    asm volatile("sti");

    thread_yield();

    return new_proc->pid;
}
