#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vfs.h>

// remove
#include <stdio.h>

static int
sys_mkpty(syscall_args_t argv)
{

    TRACE_SYSCALL("mkpty", "void");

    extern struct file *mkpty();

    struct file *fp = mkpty();

    if (!fp) {
        return -1;
    }
    
    int fd = proc_getfildes();

    if (fd < 0) {
        fops_close(fp);
        return -1;
    } else {
        current_proc->files[fd] = fp;
    }

    return fd;
}


__attribute__((constructor))
void
_init_pty_syscalls()
{
    register_syscall(SYS_MKPTY, 0, sys_mkpty);
}
