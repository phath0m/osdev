#include <rtl/types.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
// remove me
#include <rtl/printf.h>

/*
 * File descriptor table manipulation
 */

struct file *
proc_getfile(struct proc *proc, int fildes)
{
    if (fildes >= 4096) {
        return NULL;
    }

    return proc->files[fildes];
}

int
proc_getfildes(struct proc *proc)
{
    for (int i = 0; i < 4096; i++) {
        if (!proc->files[i]) {
            return i;
        }
    }

    return -EMFILE;
}

int
proc_fstat(struct proc *proc, int fd, struct stat *buf)
{
    struct file *file = proc_getfile(proc, fd);

    if (file) {
        return vfs_stat(file, buf);
    }

    return -(EBADF);
}

off_t
proc_lseek(struct proc *proc, int fd, off_t offset, int whence)
{
    struct file *file = proc_getfile(proc, fd);

    if (file) {
        return vfs_seek(file, offset, whence);
    }

    return -(EBADF);
}

int
proc_open(struct proc *proc, const char *path, int mode)
{
    struct file *file;

    int succ = vfs_open(proc->root, &file, path, mode);

    if (succ == 0) {
        int fd = proc_getfildes(proc);

        if (fd < 0) {
            vfs_close(file);
        } else {
            proc->files[fd] = file;
        }

        return fd;
    }
    return -(succ);
}

int
proc_read(struct proc *proc, int fildes, char *buf, size_t nbyte)
{
    struct file *file = proc_getfile(proc, fildes);

    if (file) {
        return vfs_read(file, buf, nbyte);
    }

    return -(EBADF);
}

int
proc_write(struct proc *proc, int fildes, const char *buf, size_t nbyte)
{
    struct file *file = proc_getfile(proc, fildes);

    if (file) {
        return vfs_write(file, buf, nbyte);
    }

    return -(EBADF);
}

/*
 * filesystem syscalls
 */

static int
sys_fstat(struct proc *proc, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    return proc_fstat(proc, fd, buf);
}

static int
sys_open(struct proc *proc, syscall_args_t argv)
{
    const char *file    = DECLARE_SYSCALL_PARAM(const char*, 0, argv);
    int mode            = DECLARE_SYSCALL_PARAM(int, 1, argv);


    return proc_open(proc, file, mode);
}

static int
sys_read(struct proc *proc, syscall_args_t argv)
{
    int fildes  = DECLARE_SYSCALL_PARAM(int, 0, argv);
    char *buf   = DECLARE_SYSCALL_PARAM(char*, 1, argv);
    size_t len  = DECLARE_SYSCALL_PARAM(size_t, 2, argv);

    return proc_read(proc, fildes, buf, len);
}

static int
sys_lseek(struct proc *proc, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, offset, 1, argv);
    DEFINE_SYSCALL_PARAM(int, whence, 2, argv);

    return proc_lseek(proc, fd, offset, whence);
}

static int
sys_stat(struct proc *proc, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    struct file *file;

    int res = 0;

    if (vfs_open(proc->root, &file, path, O_RDONLY) == 0) {
        res = vfs_stat(file, buf);

        vfs_close(file);
    }

    return res;
}

static int
sys_write(struct proc *proc, syscall_args_t argv)
{
    int fildes      = DECLARE_SYSCALL_PARAM(int, 0, argv);
    const char *buf = DECLARE_SYSCALL_PARAM(const char *, 1, argv);
    size_t len      = DECLARE_SYSCALL_PARAM(size_t, 2, argv);

    return proc_write(proc, fildes, buf, len);
}

__attribute__((constructor)) static void
_init_syscalls()
{
    register_syscall(SYS_FSTAT, 2, sys_fstat);
    register_syscall(SYS_LSEEK, 3, sys_lseek);
    register_syscall(SYS_OPEN, 2, sys_open);
    register_syscall(SYS_READ, 3, sys_read);
    register_syscall(SYS_STAT, 2, sys_stat);
    register_syscall(SYS_WRITE, 3, sys_write);
}
