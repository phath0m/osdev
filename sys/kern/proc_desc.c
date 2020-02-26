/*
 * proc_desc.c
 *
 * Responsible for the management of a process's file descriptor table
 *
 */
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/vnode.h>

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
        vops_close(existing_fp);   
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
    
    return -(EMFILE);
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

    return -(EMFILE);
}

static int
get_fcntl_flags(struct file *fp)
{
    int ret = 0;

    if ((fp->flags & O_CLOEXEC)) {
        ret |= FD_CLOEXEC; 
    }

    return ret;
}

static int
set_fcntl_flags(struct file *fp, int flags)
{
    if (flags & FD_CLOEXEC) {
        fp->flags |= O_CLOEXEC;
    }

    return 0;
}

int
proc_fcntl(int fd, int cmd, void *arg)
{
    if (fd >= 4096 || fd >= 4096) {
        return -1;
    }

    struct file *fp = current_proc->files[fd];

    if (!fp) {
        return -1;
    }

    switch (cmd) {
        case F_SETFD:
            return set_fcntl_flags(fp, *((int*)arg));
        case F_GETFD:

            if (arg) {
                *((int*)arg) = get_fcntl_flags(fp);
            }

            return 0;
    }

    return -1;
}

