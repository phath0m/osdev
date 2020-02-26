/*
 * pty_syscalls.c
 *
 * This file is responsible for implementing terminal system calls
 *
 */
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vnode.h>

static int
sys_isatty(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);

    TRACE_SYSCALL("isatty", "%d", fildes);

    struct file *file = procdesc_getfile(fildes);

    if (!file) {
        return -(EBADF);
    }

    struct vnode *node = file->node;

    if (!node) {
        return -(ENOTTY);
    }

    struct cdev *tty = file->node->device;

    if (!node) {
        return -(ENOTTY);
    }

    if (!cdev_isatty(tty)) {
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

    struct file *file = procdesc_getfile(fildes);

    if (!file) {
        return -(EBADF);
    }

    struct vnode *node = file->node;

    if (!node) {
        return -(ENOTTY);
    }

    struct cdev *tty = file->node->device;

    if (!node) {
        return -(ENOTTY);
    }

    if (!cdev_isatty(tty)) {
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
    
    int fd = procdesc_getfd();

    if (fd < 0) {
        vops_close(fp);
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
