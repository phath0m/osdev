#include <stdlib.h>
#include <ds/list.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vm.h>

static int next_pid;

/*
 * List of all processes running
 */
struct list process_list;

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

    list_remove(&process_list, proc);

    free(proc);
}

struct file *
proc_getfile(int fildes)
{   
    if (fildes >= 4096) {
        return NULL;
    }
    return current_proc->files[fildes];
}

int
proc_getfildes()
{   
    for (int i = 0; i < 4096; i++) { 
        if (!current_proc->files[i]) {
            return i;
        }
    }
    
    return -EMFILE;
}

struct proc *
proc_new()
{
    struct proc *proc = (struct proc*)calloc(0, sizeof(struct proc));
    proc->pid = ++next_pid;

    list_append(&process_list, proc);

    return proc;
}

int
proc_newfildes(struct file *file)
{
    for (int i = 0; i < 4096; i++) {
        if (!current_proc->files[i]) {
            current_proc->files[i] = file;
            return i;
        }
    }

    return -EMFILE;
}

