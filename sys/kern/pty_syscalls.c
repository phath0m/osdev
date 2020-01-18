#include <string.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vfs.h>

// remove
#include <stdio.h>

static int
sys_isatty(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);

    TRACE_SYSCALL("isatty", "%d", fildes);

    struct file *file = proc_getfile(fildes);

    if (!file) {
        return -(EBADF);
    }

    struct vfs_node *node = file->node;

    if (!node) {
        return -(ENOTTY);
    }

    struct device *tty = file->node->device;

    if (!node) {
        return -(ENOTTY);
    }

    if (!device_isatty(tty)) {
        return -(ENOTTY);
    }

    return 0;
}

static int
sys_ttyname(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(char *, buf, 1, argv);

    TRACE_SYSCALL("ttyname", "%d, 0x%p", fildes, buf);

    struct file *file = proc_getfile(fildes);

    if (!file) {
        return -(EBADF);
    }

    struct vfs_node *node = file->node;

    if (!node) {
        return -(ENOTTY);
    }

    struct device *tty = file->node->device;

    if (!node) {
        return -(ENOTTY);
    }

    if (!device_isatty(tty)) {
        return -(ENOTTY);
    }

    // this is nasty... we're ignoring the length of buf and
    // also assuming its mounted to /dev. this is just a temporary
    // hack...
    sprintf(buf ,"/dev/%s", tty->name);

    return 0; 
}

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
    register_syscall(SYS_ISATTY, 1, sys_isatty);
    register_syscall(SYS_MKPTY, 0, sys_mkpty);
    register_syscall(SYS_TTYNAME, 3, sys_ttyname);
}
