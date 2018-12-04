#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vfs.h>

// remove
#include <stdio.h>

int
sys_close(int fd)
{
    struct file *fp = proc_getfile(fd);

    if (fp) {
        current_proc->files[fd] = NULL;

        vfs_close(fp);
        return 0;
    }

    return -(EBADF);
}

int
sys_fstat(int fd, struct stat *buf)
{
    struct file *file = proc_getfile(fd);

    if (file) {
        return vfs_stat(file, buf);
    }

    return -(EBADF);
}

off_t
sys_lseek(int fd, off_t offset, int whence)
{
    struct file *file = proc_getfile(fd);

    if (file) {
        return vfs_seek(file, offset, whence);
    }

    return -(EBADF);
}

int
sys_open(const char *path, int mode)
{
    struct file *file;

    int succ = vfs_open_r(current_proc->root, current_proc->cwd, &file, path, mode);

    if (succ == 0) {
        int fd = proc_getfildes();

        if (fd < 0) {
            vfs_close(file);
        } else {
            current_proc->files[fd] = file;
        }

        return fd;
    }
    return -(succ);
}

int
sys_pipe(int pipefd[2])
{
    struct file *files[2];

    create_pipe(files);

    pipefd[0] = proc_newfildes(files[0]);
    pipefd[1] = proc_newfildes(files[1]);

    return 0;
}

int
sys_read(int fildes, char *buf, size_t nbyte)
{
    struct file *file = proc_getfile(fildes);

    if (file) {
        return vfs_read(file, buf, nbyte);
    }

    return -(EBADF);
}

int
sys_readdir(int fildes, struct dirent *dirent)
{
    struct file *file = proc_getfile(fildes);

    if (file) {
        return vfs_readdirent(file, dirent);
    }

    return -(EBADF);
}

int
sys_write(int fildes, const char *buf, size_t nbyte)
{
    struct file *file = proc_getfile(fildes);

    if (file) {
        return vfs_write(file, buf, nbyte);
    }

    return -(EBADF);
}

/*
 * filesystem syscalls
 */

static int
sys_close_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);

    return sys_close(fd);
}

static int
sys_fstat_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    return sys_fstat(fd, buf);
}

static int
sys_open_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, file, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    return sys_open(file, mode);
}

static int
sys_pipe_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int *, pipefd, 0, argv);

    return sys_pipe(pipefd);
}

static int
sys_read_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(char*, buf, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, len, 2, argv);

    asm volatile("sti");

    return sys_read(fildes, buf, len);
}

static int
sys_readdir_handler(syscall_args_t argv)
{
    int fildes              = DECLARE_SYSCALL_PARAM(int, 0, argv);
    struct dirent *dirent   = DECLARE_SYSCALL_PARAM(struct dirent *, 1, argv);
    return sys_readdir(fildes, dirent);
}

static int
sys_lseek_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, offset, 1, argv);
    DEFINE_SYSCALL_PARAM(int, whence, 2, argv);

    return sys_lseek(fd, offset, whence);
}

static int
sys_stat_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    struct file *file;

    int res = 0;

    if (vfs_open_r(current_proc->root, current_proc->cwd, &file, path, O_RDONLY) == 0) {
        res = vfs_stat(file, buf);

        vfs_close(file);
    }

    return res;
}

static int
sys_write_handler(syscall_args_t argv)
{
    int fildes      = DECLARE_SYSCALL_PARAM(int, 0, argv);
    const char *buf = DECLARE_SYSCALL_PARAM(const char *, 1, argv);
    size_t len      = DECLARE_SYSCALL_PARAM(size_t, 2, argv);

    asm volatile("sti");

    return sys_write(fildes, buf, len);
}

__attribute__((constructor))
void
_init_vfs_syscalls()
{
    register_syscall(SYS_CLOSE, 1, sys_close_handler);
    register_syscall(SYS_FSTAT, 2, sys_fstat_handler);
    register_syscall(SYS_LSEEK, 3, sys_lseek_handler);
    register_syscall(SYS_OPEN, 2, sys_open_handler);
    register_syscall(SYS_PIPE, 1, sys_pipe_handler);
    register_syscall(SYS_READ, 3, sys_read_handler);
    register_syscall(SYS_READDIR, 2, sys_readdir_handler);
    register_syscall(SYS_STAT, 2, sys_stat_handler);
    register_syscall(SYS_WRITE, 3, sys_write_handler);
}
