#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/syscalls.h>
#include <sys/time.h>
#include <stdio.h>

char **environ;

void
_exit(int status)
{
    asm volatile("int $0x80" : : "a"(SYS_EXIT), "b"(status));
}

int
chdir(const char *path)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CHDIR), "b"(path));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
close(int file)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLOSE), "b"(file));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
creat(const char *path, mode_t mode)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CREAT), "b"(path), "c"(mode));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
connect(int fd, const struct sockaddr *address, socklen_t address_size)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "A"(SYS_CONNECT), "b"(fd), "c"(address), "d"(address_size));

    return ret;
}

int
dup(int oldfd)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "A"(SYS_DUP), "b"(oldfd));

    return ret;
}

int
dup2(int oldfd, int newfd)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "A"(SYS_DUP2), "b"(oldfd), "c"(newfd));

    return ret;
}

int
execv(char *name, char **argv)
{
    return execve(name, argv, environ);
}

int
execve(char *name, char **argv, char **env)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_EXECVE), "b"(name), "c"(argv), "d"(env));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
fork()
{
    int ret;
    
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FORK));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
fstat(int file, struct stat *st)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FSTAT), "b"(file), "c"(st));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getpid()
{
    return -1;
}

int
isatty(int file)
{
    return 1;
}

int
kill(int pid, int sig)
{
    return -1;
}

int
link(char *old, char *new)
{
    return -1;
}

int
lseek(int file, int ptr, int dir)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_LSEEK), "b"(file), "c"(ptr), "d"(dir));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
mkdir(const char *path, mode_t mode)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_MKDIR), "b"(path), "c"(mode));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
open(const char *name, int flags, ...)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_OPEN), "b"(name), "c"(flags));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
pipe(int pipefd[2])
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_PIPE), "b"(pipefd));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
read(int file, char *ptr, int len)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_READ), "b"(file), "c"(ptr), "d"(len));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
rmdir(const char *path)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_RMDIR), "b"(path));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

caddr_t
sbrk(int incr)
{
    caddr_t ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SBRK), "b"(incr));

    return ret;
}

int
socket(int domain, int type, int protocol)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "A"(SYS_SOCKET), "b"(domain), "c"(type), "d"(protocol));

    return ret;
}

int
stat(const char *file, struct stat *st)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_STAT), "b"(file), "c"(st));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

clock_t
times(struct tms *buf)
{

}

mode_t
umask(mode_t newmode)
{
    mode_t ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_UMASK), "b"(newmode));

    return ret;
}

int
unlink(char *path)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_UNLINK), "b"(path));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
wait(int *status)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_WAIT), "b"(status));

    if (ret < 0) {
        errno = -ret;

        return -1;
    }

    return ret;
}

int
waitpid(pid_t pid, int *status)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_WAITPID), "b"(pid), "c"(status));

    if (ret < 0) {
        errno = -ret;

        return -1;
    }

    return ret;
}

int
write(int file, char *ptr, int len)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_WRITE), "b"(file), "c"(ptr), "d"(len));

    if (ret < 0) {
        errno = -ret;

        return -1;
    }

    return ret;
}

int
gettimeofday(struct timeval *p, void *z)
{
    return -1;
}
