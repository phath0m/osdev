#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

#define SYS_READ        0x00
#define SYS_WRITE       0x01
#define SYS_OPEN        0x02
#define SYS_CLOSE       0x03
#define SYS_STAT        0x04
#define SYS_FSTAT       0x05
#define SYS_LSEEK       0x06
#define SYS_FNCTL       0x07
#define SYS_IOCTL       0x08
#define SYS_SBRK        0x09
#define SYS_ACCESS      0x0A
#define SYS_EXECVE      0x0B
#define SYS_FORK        0x0C
#define SYS_EXIT        0x0D
#define SYS_UNAME       0x0E

char **environ;

void
_exit()
{
}

int
close(int file)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLOSE), "b"(file));

    return ret;
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
    return -1;
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
