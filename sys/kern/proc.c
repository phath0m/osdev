#include <stdlib.h>
#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vm.h>

static int next_pid;

int proc_count = 0;
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
            fops_close(fp);
        }
    }

    vm_space_destroy(thread->address_space);

    list_remove(&process_list, proc);

    proc_count--;

    free(proc);
}

int
proc_dup(int oldfd)
{
    int newfd = proc_getfildes();

    return proc_dup2(oldfd, newfd);
}

int
proc_dup2(int oldfd, int newfd)
{
    if (oldfd >= 4096 || newfd >= 4096) {
        return -1;
    }

    if (oldfd < 0 || newfd < 0) {
        return -1;
    }

    struct file *existing_fp = current_proc->files[newfd];

    if (existing_fp) {
        fops_close(existing_fp);   
    }

    struct file *fp = current_proc->files[oldfd];
   
    if (!fp) {
        return -1;
    }

    current_proc->files[newfd] = fp;

    INC_FILE_REF(fp);
    
    return newfd;
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

char *
proc_getctty(struct proc *proc)
{
    if (proc->files[0]) {
        struct file *file = proc->files[0];

        if (!file || !file->node || !file->node->device) {
            return NULL;
        }

        if (device_isatty(file->node->device)) {
            return file->node->device->name;
        }
    }

    return NULL;
}

struct proc *
proc_new()
{
    struct proc *proc = (struct proc*)calloc(0, sizeof(struct proc));
    
    proc->pid = ++next_pid;

    list_append(&process_list, proc);

    proc_count++;

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

