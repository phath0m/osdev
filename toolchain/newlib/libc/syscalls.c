#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
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

    return ret;
}

int
close(int file)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLOSE), "b"(file));

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

    return ret;
}

int
fork()
{
    int ret;
    
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FORK));

    return ret;
}

int
fstat(int file, struct stat *st)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FSTAT), "b"(file), "c"(st));

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

    return ret;
}

int
open(const char *name, int flags, ...)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_OPEN), "b"(name), "c"(flags));

    return ret;
}

int
pipe(int pipefd[2])
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_PIPE), "b"(pipefd));

    return ret;
}

int
read(int file, char *ptr, int len)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_READ), "b"(file), "c"(ptr), "d"(len));

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
stat(const char *file, struct stat *st)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_STAT), "b"(file), "c"(st));

    return ret;
}

clock_t
times(struct tms *buf)
{

}

int
unlink(char *name)
{
    return -1;
}

int
wait(int *status)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_WAIT), "b"(status));

    return ret;
}

int
waitpid(pid_t pid, int *status)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_WAITPID), "b"(pid), "c"(status));

    return ret;
}

int
write(int file, char *ptr, int len)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_WRITE), "b"(file), "c"(ptr), "d"(len));

    return ret;
}

int
gettimeofday(struct timeval *p, void *z)
{
    return -1;
}
