#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vfs.h>

// remove
#include <stdio.h>


/*
 * filesystem syscalls
 */

static int
sys_close_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);

    TRACE_SYSCALL("close", "%d", fd);

    struct file *fp = proc_getfile(fd);

    if (fp) {
        current_proc->files[fd] = NULL;

        vfs_close(fp);
        return 0;
    }

    return -(EBADF);
}

static int
sys_creat_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    TRACE_SYSCALL("creat", "\"%s\", %d", path, mode);

    struct file *file;

    int succ = vfs_creat(current_proc->root, current_proc->cwd, &file, path, mode);

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

static int
sys_fstat_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    TRACE_SYSCALL("fstat", "%d, %p", fd, buf);

    struct file *file = proc_getfile(fd);

    if (file) {
        return vfs_stat(file, buf);
    }

    return -(EBADF);
}

static int
sys_open_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    TRACE_SYSCALL("open", "\"%s\", %d", path, mode);

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

static int
sys_pipe_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int *, pipefd, 0, argv);

    TRACE_SYSCALL("pipe", "%p", pipefd);

    struct file *files[2];

    create_pipe(files);

    pipefd[0] = proc_newfildes(files[0]);
    pipefd[1] = proc_newfildes(files[1]);

    return 0;
}

static int
sys_read_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(char*, buf, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, nbyte, 2, argv);

    TRACE_SYSCALL("read", "%d, %p, %d", fildes, buf, nbyte);

    asm volatile("sti");

    struct file *file = proc_getfile(fildes);

    if (file) {
        return vfs_read(file, buf, nbyte);
    }

    return -(EBADF);
}

static int
sys_readdir_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(struct dirent *, dirent, 1, argv);

    TRACE_SYSCALL("readdir", "%d, %p", fildes, dirent);

    struct file *file = proc_getfile(fildes);

    if (file) {
        return vfs_readdirent(file, dirent);
    }

    return -(EBADF);
}

static int
sys_rmdir_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);

    TRACE_SYSCALL("rmdir", "\"%s\"", path);

    return vfs_rmdir(current_proc->root, current_proc->cwd, path);
}

static int
sys_lseek_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, offset, 1, argv);
    DEFINE_SYSCALL_PARAM(int, whence, 2, argv);

    TRACE_SYSCALL("lseek", "%d, %d, %d", fd, offset, whence);

    struct file *file = proc_getfile(fd);

    if (file) {
        return vfs_seek(file, offset, whence);
    }

    return -(EBADF);
}

static int
sys_mkdir_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 0, argv);
    
    TRACE_SYSCALL("mkdir", "\"%s\", %d", path, mode);

    return vfs_mkdir(current_proc->root, current_proc->cwd, path, mode);
}

static int
sys_stat_handler(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    TRACE_SYSCALL("stat", "\"%s\", %p", path, buf);

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
    size_t nbyte      = DECLARE_SYSCALL_PARAM(size_t, 2, argv);

    TRACE_SYSCALL("write", "%d, %p, %d", fildes, buf, nbyte);

    asm volatile("sti");

    struct file *file = proc_getfile(fildes);

    if (file) {
        return vfs_write(file, buf, nbyte);
    }

    return -(EBADF);
}

__attribute__((constructor))
void
_init_vfs_syscalls()
{
    register_syscall(SYS_CLOSE, 1, sys_close_handler);
    register_syscall(SYS_CREAT, 2, sys_creat_handler);
    register_syscall(SYS_FSTAT, 2, sys_fstat_handler);
    register_syscall(SYS_LSEEK, 3, sys_lseek_handler);
    register_syscall(SYS_OPEN, 2, sys_open_handler);
    register_syscall(SYS_PIPE, 1, sys_pipe_handler);
    register_syscall(SYS_READ, 3, sys_read_handler);
    register_syscall(SYS_READDIR, 2, sys_readdir_handler);
    register_syscall(SYS_RMDIR, 1, sys_rmdir_handler);
    register_syscall(SYS_MKDIR, 2, sys_mkdir_handler);
    register_syscall(SYS_STAT, 2, sys_stat_handler);
    register_syscall(SYS_WRITE, 3, sys_write_handler);
}
