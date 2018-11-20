#include <stdlib.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vm.h>

static int next_pid;

void
proc_destroy(struct proc *proc)
{
    /*
     * todo: terminate all threads and unmap virtual memory
     */
    struct thread *thread = proc->thread;

    for (int i = 0; i < 4096; i++) {
        struct file *fp = proc->files[i];

        if (fp) {
            vfs_close(fp);
        }
    }

    vm_space_destroy(thread->address_space);

    free(proc);
}

struct proc *
proc_new()
{
    struct proc *proc = (struct proc*)calloc(0, sizeof(struct proc));
    proc->pid = ++next_pid;
    return proc;
}
