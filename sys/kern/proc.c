#include <rtl/malloc.h>
#include <sys/proc.h>
#include <sys/vm.h>

static int next_pid;

static void
destroy_thread(struct thread *thread)
{
    size_t stack_size = thread->u_stack_top - thread->u_stack_bottom;

    vm_unmap(thread->address_space, (void*)thread->u_stack_bottom, stack_size);
}

void
proc_destroy(struct proc *proc)
{
    /*
     * todo: terminate all threads and unmap virtual memory
     */
    struct thread *thread = proc->thread;

    destroy_thread(thread);

    size_t image_size = proc->brk - proc->base;

    vm_unmap(thread->address_space, (void*)proc->base, image_size);
    
    free(proc);
}

struct proc *
proc_new()
{
    struct proc *proc = (struct proc*)calloc(0, sizeof(struct proc));
    proc->pid = ++next_pid;
    return proc;
}
