/*
 * vfs_syscalls.c
 *
 * This file is responsible for implementing generic filesystem system calls
 *
 */
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>

static int
sys_access(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    TRACE_SYSCALL("access", "\"%s\", %d", path, mode);

    return vops_access(current_proc, path, mode);
}

static int
sys_chmod(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 1, argv);

    TRACE_SYSCALL("chmod", "\"%s\", %d", path, mode);

    return vops_chmod(current_proc, path, mode);
}

static int
sys_chown(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(uid_t, owner, 1, argv);
    DEFINE_SYSCALL_PARAM(gid_t, group, 2, argv);

    TRACE_SYSCALL("chown", "\"%s\", %d, %d", path, owner, group);

    return vops_chown(current_proc, path, owner, group);
}

static int
sys_close(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);

    TRACE_SYSCALL("close", "%d", fd);

    struct file *fp = procdesc_getfile(fd);

    if (fp) {
        current_proc->files[fd] = NULL;

        vops_close(fp);

        return 0;
    }

    return -(EBADF);
}

static int
sys_creat(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    TRACE_SYSCALL("creat", "\"%s\", %d", path, mode);

    struct file *file;
    mode &= current_proc->umask;

    int succ = vops_creat(current_proc, &file, path, mode);

    if (succ == 0) {
        int fd = procdesc_getfd();

        if (fd < 0) {
            vops_close(file);
        } else {
            current_proc->files[fd] = file;
        }

        return fd;
    }

    return -(succ);
}

static int
sys_fchmod(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 1, argv);

    TRACE_SYSCALL("fchmod", "%d, %d", fd, mode);

    struct file *file = procdesc_getfile(fd);

    if (file) {
        return vops_fchmod(file, mode);
    }

    return -(EBADF);
}

static int
sys_fchown(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(uid_t, owner, 1, argv);
    DEFINE_SYSCALL_PARAM(gid_t, group, 2, argv);

    TRACE_SYSCALL("fchown", "%d, %d, %d", fd, owner, group);

    struct file *file = procdesc_getfile(fd);

    if (file) {
        return vops_fchown(file, owner, group);
    }

    return -(EBADF);
}

static int
sys_ftruncate(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, length, 1, argv);

    TRACE_SYSCALL("ftruncate", "%d, 0x%p", fd, length);

    struct file *file = procdesc_getfile(fd);

    if (file) {
        return vops_ftruncate(file, length);
    }

    return -(EBADF);
}

static int
sys_ioctl(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(uint32_t, request, 1, argv);
    DEFINE_SYSCALL_PARAM(void*, arg, 2, argv);

    TRACE_SYSCALL("ioctl", "%d, %d, %p", fd, request, arg);

    struct file *file = procdesc_getfile(fd);

    if (file) {
        asm volatile("sti");
        return vops_ioctl(file, (uint64_t)request, arg);
    }

    return -(EBADF);
}

static int
sys_fstat(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    TRACE_SYSCALL("fstat", "%d, %p", fd, buf);

    struct file *file = procdesc_getfile(fd);

    if (file) {
        return vops_stat(file, buf);
    }

    return -(EBADF);
}

static int
sys_open(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    TRACE_SYSCALL("open", "\"%s\", %d", path, mode);

    struct file *file;

    int succ = vops_open_r(current_proc, &file, path, mode);

    if (succ == 0) {
        int fd = procdesc_getfd();

        if (fd < 0) {
            vops_close(file);
        } else {
            current_proc->files[fd] = file;
        }

        return fd;
    }

    return succ;
}

static int
sys_pipe(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int *, pipefd, 0, argv);

    TRACE_SYSCALL("pipe", "%p", pipefd);

    struct file *files[2];

    create_pipe(files);

    pipefd[0] = procdesc_newfd(files[0]);
    pipefd[1] = procdesc_newfd(files[1]);

    return 0;
}

static int
sys_read(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(char*, buf, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, nbyte, 2, argv);

    TRACE_SYSCALL("read", "%d, %p, %d", fildes, buf, nbyte);

    asm volatile("sti");

    struct file *file = procdesc_getfile(fildes);

    if (file) {
        return vops_read(file, buf, nbyte);
    }

    return -(EBADF);
}

static int
sys_readdir(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(struct dirent *, dirent, 1, argv);

    TRACE_SYSCALL("readdir", "%d, %p", fildes, dirent);

    struct file *file = procdesc_getfile(fildes);

    if (file) {
        return vops_readdirent(file, dirent);
    }

    return -(EBADF);
}

static int
sys_rmdir(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);

    TRACE_SYSCALL("rmdir", "\"%s\"", path);

    return vops_rmdir(current_proc, path);
}

static int
sys_lseek(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, offset, 1, argv);
    DEFINE_SYSCALL_PARAM(int, whence, 2, argv);

    TRACE_SYSCALL("lseek", "%d, %d, %d", fd, offset, whence);

    struct file *file = procdesc_getfile(fd);

    if (file) {
        return vops_seek(file, offset, whence);
    }

    return -(EBADF);
}

static int
sys_mkdir(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 1, argv);
    
    TRACE_SYSCALL("mkdir", "\"%s\", %x", path, mode);

    mode &= current_proc->umask;

    return vops_mkdir(current_proc, path, mode);
}

static int
sys_mknod(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(mode_t, mode, 1, argv);
    DEFINE_SYSCALL_PARAM(dev_t, dev, 2, argv);
    
    TRACE_SYSCALL("mknod", "\"%s\", %x, %d", path, mode, dev);

    mode_t extra = mode & ~(0777);
    mode &= current_proc->umask;
    mode |= extra;

    return vops_mknod(current_proc, path, mode, dev);
}

static int
sys_stat(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    TRACE_SYSCALL("stat", "\"%s\", %p", path, buf);

    struct file *file;

    int res = vops_open_r(current_proc, &file, path, O_RDONLY);

    if (res == 0) {
        res = vops_stat(file, buf);

        vops_close(file);
    }

    return res;
}

static int
sys_truncate(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, length, 1, argv);

    TRACE_SYSCALL("truncate", "\"%s\", %d", path, length);

    return vops_truncate(current_proc, path, length);
}

static int
sys_unlink(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);

    TRACE_SYSCALL("unlink", "\"%s\"", path);

    return vops_unlink(current_proc, path);
}

static int
sys_utimes(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, argv);
    DEFINE_SYSCALL_PARAM(struct timeval *, tv, 1, argv);

    return vops_utimes(current_proc, path, tv);
}

static int
sys_write(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(const char *, buf, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, nbyte, 2, argv);

    TRACE_SYSCALL("write", "%d, %p, %d", fildes, buf, nbyte);

    asm volatile("sti");

    struct file *file = procdesc_getfile(fildes);

    if (file) {
        return vops_write(file, buf, nbyte);
    }

    return -(EBADF);
}

__attribute__((constructor))
void
_init_vfs_syscalls()
{
    register_syscall(SYS_CLOSE, 1, sys_close);
    register_syscall(SYS_CREAT, 2, sys_creat);
    register_syscall(SYS_CHMOD, 2, sys_chmod);
    register_syscall(SYS_FCHMOD, 2, sys_fchmod);
    register_syscall(SYS_FSTAT, 2, sys_fstat);
    register_syscall(SYS_IOCTL, 3, sys_ioctl);
    register_syscall(SYS_LSEEK, 3, sys_lseek);
    register_syscall(SYS_OPEN, 2, sys_open);
    register_syscall(SYS_PIPE, 1, sys_pipe);
    register_syscall(SYS_READ, 3, sys_read);
    register_syscall(SYS_READDIR, 2, sys_readdir);
    register_syscall(SYS_RMDIR, 1, sys_rmdir);
    register_syscall(SYS_MKDIR, 2, sys_mkdir);
    register_syscall(SYS_STAT, 2, sys_stat);
    register_syscall(SYS_UNLINK, 1, sys_unlink);
    register_syscall(SYS_WRITE, 3, sys_write);
    register_syscall(SYS_CHOWN, 3, sys_chown);
    register_syscall(SYS_FCHOWN, 3, sys_fchown);
    register_syscall(SYS_TRUNCATE, 2, sys_truncate);
    register_syscall(SYS_FTRUNCATE, 2, sys_ftruncate);
    register_syscall(SYS_ACCESS, 2, sys_access);
    register_syscall(SYS_MKNOD, 3, sys_mknod);
    register_syscall(SYS_UTIMES, 2, sys_utimes);
}
