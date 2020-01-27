#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/syscalls.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <utime.h>

char **environ;


/* Begin things that I've been too lazy to implement */
long
sysconf(int name)
{
    switch (name) {
        case 8:
            return 4096;
        case 11:
            return 10000;
        default:
            return -1;
    }
}

int
fcntl(int fd, int cmd, ...)
{
    if (cmd == F_GETFD || cmd == F_SETFD) {
        return 0;
    }

    return -1;
}


long
pathconf(char *path, int name)
{
    return 0;
}

int
fpathconf(int file, int name)
{
    return 0;
}

char *
getcwd(char *buf, size_t size)
{
    return "/";
}

char *
getwd(char *buf)
{
    return getcwd(buf, 256);
}

int
utime(const char *filename, const struct utimbuf *times)
{
    return 0;
}

unsigned int
sleep(unsigned int seconds)
{
    asm volatile("int $0x80" : : "a"(SYS_SLEEP), "b"(seconds));
    return 0;
}

char *
getlogin(void)
{
    return "root";
}

/* here are the actual syscalls */

void
_exit(int status)
{
    asm volatile("int $0x80" : : "a"(SYS_EXIT), "b"(status));
}

int
access(const char *path, int mode)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_ACCESS), "b"(path), "c"(mode));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
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
chmod(const char *path, mode_t mode)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CHMOD), "b"(path), "c"(mode));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
chown(const char *path, uid_t owner, gid_t group)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CHOWN), "b"(path), "c"(owner), "d"(group));

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
fchmod(int file, mode_t mode)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FCHMOD), "b"(file), "c"(mode));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
fchown(int file, uid_t owner, gid_t group)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FCHOWN), "b"(file), "c"(owner), "d"(group));

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
ftruncate(int file, off_t length)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FTRUNCATE), "b"(file), "c"(length));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getegid()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETEGID));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
geteuid()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETEUID));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getgid()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETGID));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getpgrp(pid_t pid)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETPGRP), "b"(pid));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getpid()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETPID));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getppid()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETPPID));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getsid()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETSID));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
gettimeofday(struct timeval *p, void *z)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_TIME), "b"(0));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    p->tv_sec = (time_t)ret;

    return 0;
}

int
getuid()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETUID));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
ioctl(int fd, unsigned long request, void *arg)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_IOCTL), "b"(fd), "c"((uint32_t)request), "d"(arg));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
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
lstat(const char *file, struct stat *st)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_STAT), "b"(file), "c"(st));

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
mkpty()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_MKPTY));

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
pause()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_PAUSE));

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
setegid(gid_t gid)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SETEGID), "b"(gid));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
seteuid(uid_t uid)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SETEUID), "b"(uid));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
setgid(gid_t gid)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SETGID), "b"(gid));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
setpgid(pid_t pid, pid_t pgid)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SETPGID), "b"(pid), "c"(pgid));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
setsid()
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SETSID));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
setuid(uid_t uid)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SETUID), "b"(uid));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

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

int
truncate(const char *path, off_t length)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_TRUNCATE), "b"(path), "c"(length));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
ttyname_r(int fd, char *buf, size_t buflen)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_TTYNAME), "b"(fd), "c"(buf), "d"(buflen));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

char *
ttyname(int fd)
{
    static char tty_name[1024];

    if (ttyname_r(fd, tty_name, sizeof(tty_name) != 0)) {
        return NULL;
    }

    return tty_name;
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
uname(struct utsname *buf)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_UNAME), "b"(buf));

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
