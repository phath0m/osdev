#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/net.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vfs.h>

// remove
#include <stdio.h>

/*
 * File descriptor table manipulation
 */
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

int
proc_close(int fd)
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
proc_connect(int fd, void *address, size_t address_len)
{
    struct file *fp = proc_getfile(fd);
    
    if (fp) {
        struct socket *sock = file_to_sock(fp);

        return sock_connect(sock, address, address_len);
    }
    
    return -(EBADF);
}

int
proc_fstat(int fd, struct stat *buf)
{
    struct file *file = proc_getfile(fd);

    if (file) {
        return vfs_stat(file, buf);
    }

    return -(EBADF);
}

off_t
proc_lseek(int fd, off_t offset, int whence)
{
    struct file *file = proc_getfile(fd);

    if (file) {
        return vfs_seek(file, offset, whence);
    }

    return -(EBADF);
}

int
proc_open(const char *path, int mode)
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
proc_pipe(int pipefd[2])
{
    struct file *files[2];

    create_pipe(files);

    pipefd[0] = proc_newfildes(files[0]);
    pipefd[1] = proc_newfildes(files[1]);

    return 0;
}

int
proc_read(int fildes, char *buf, size_t nbyte)
{
    struct file *file = proc_getfile(fildes);

    if (file) {
        return vfs_read(file, buf, nbyte);
    }

    return -(EBADF);
}

int
proc_readdir(int fildes, struct dirent *dirent)
{
    struct file *file = proc_getfile(fildes);

    if (file) {
        return vfs_readdirent(file, dirent);
    }

    return -(EBADF);
}

int
proc_socket(int domain, int type, int protocol)
{
    struct socket *sock;

    int ret = sock_new(&sock, domain, type, protocol);

    if (ret != 0) {
        return ret;
    }

    struct file *file = sock_to_file(sock);

    return proc_newfildes(file);
}

int
proc_write(int fildes, const char *buf, size_t nbyte)
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
sys_close(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);

    return proc_close(fd);
}

static int
sys_connect(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(void*, address, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, address_len, 2, argv);

    return proc_connect(fd, address, address_len);
}

static int
sys_fstat(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(struct stat *, buf, 1, argv);

    return proc_fstat(fd, buf);
}

static int
sys_open(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(const char *, file, 0, argv);
    DEFINE_SYSCALL_PARAM(int, mode, 1, argv);

    return proc_open(file, mode);
}

static int
sys_pipe(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int *, pipefd, 0, argv);

    return proc_pipe(pipefd);
}

static int
sys_read(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fildes, 0, argv);
    DEFINE_SYSCALL_PARAM(char*, buf, 1, argv);
    DEFINE_SYSCALL_PARAM(size_t, len, 2, argv);

    asm volatile("sti");

    return proc_read(fildes, buf, len);
}

static int
sys_readdir(syscall_args_t argv)
{
    int fildes              = DECLARE_SYSCALL_PARAM(int, 0, argv);
    struct dirent *dirent   = DECLARE_SYSCALL_PARAM(struct dirent *, 1, argv);
    return proc_readdir(fildes, dirent);
}

static int
sys_lseek(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(off_t, offset, 1, argv);
    DEFINE_SYSCALL_PARAM(int, whence, 2, argv);

    return proc_lseek(fd, offset, whence);
}

static int
sys_stat(syscall_args_t argv)
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
sys_socket(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, domain, 0, argv);
    DEFINE_SYSCALL_PARAM(int, type, 1, argv);
    DEFINE_SYSCALL_PARAM(int, protocol, 2, argv);

    return proc_socket(domain, type, protocol);
}

static int
sys_write(syscall_args_t argv)
{
    int fildes      = DECLARE_SYSCALL_PARAM(int, 0, argv);
    const char *buf = DECLARE_SYSCALL_PARAM(const char *, 1, argv);
    size_t len      = DECLARE_SYSCALL_PARAM(size_t, 2, argv);

    asm volatile("sti");

    return proc_write(fildes, buf, len);
}

__attribute__((constructor)) static void
_init_syscalls()
{
    register_syscall(SYS_CLOSE, 1, sys_close);
    register_syscall(SYS_CONNECT, 3, sys_connect);
    register_syscall(SYS_FSTAT, 2, sys_fstat);
    register_syscall(SYS_LSEEK, 3, sys_lseek);
    register_syscall(SYS_OPEN, 2, sys_open);
    register_syscall(SYS_PIPE, 1, sys_pipe);
    register_syscall(SYS_READ, 3, sys_read);
    register_syscall(SYS_READDIR, 2, sys_readdir);
    register_syscall(SYS_SOCKET, 3, sys_socket);
    register_syscall(SYS_STAT, 2, sys_stat);
    register_syscall(SYS_WRITE, 3, sys_write);
}
