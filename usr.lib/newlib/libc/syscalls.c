#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/syscalls.h>
#include <sys/utsname.h>
#include <stdarg.h>
#include <stdio.h>
#include <utime.h>

char **environ;

void
_exit(int status)
{
    _SYSCALL1(void, SYS_EXIT, status);
}

int
accept(int fd, struct sockaddr *address, socklen_t *address_size)
{
    int ret = _SYSCALL3(int, SYS_ACCEPT, fd, address, address_size);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
access(const char *path, int mode)
{
    int ret = _SYSCALL2(int, SYS_ACCESS, path, mode);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
adjtime(const struct timeval *delta, struct timeval *olddelta)
{
    int ret = _SYSCALL2(int, SYS_ADJTIME, delta, olddelta);
    
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
bind(int fd, const struct sockaddr *address, socklen_t address_size)
{
    int ret = _SYSCALL3(int, SYS_BIND, fd, address, address_size);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
chdir(const char *path)
{
    int ret = _SYSCALL1(int, SYS_CHDIR, path);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
chmod(const char *path, mode_t mode)
{
    int ret = _SYSCALL2(int, SYS_CHMOD, path, mode);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
chown(const char *path, uid_t owner, gid_t group)
{
    int ret = _SYSCALL3(int, SYS_CHOWN, path, owner, group);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
close(int file)
{
    int ret = _SYSCALL1(int, SYS_CLOSE, file);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
creat(const char *path, mode_t mode)
{
    int ret = _SYSCALL2(int, SYS_CREAT, path, mode);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
connect(int fd, const struct sockaddr *address, socklen_t address_size)
{
    int ret = _SYSCALL3(int, SYS_CONNECT, fd, address, address_size);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

pid_t
clone(int (*func)(void *arg), void *stack, int flags, void *arg)
{
    int ret = _SYSCALL4(int, SYS_CLONE, func, stack, flags, arg);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return (pid_t)ret;
}

int
dup(int oldfd)
{
    int ret = _SYSCALL1(int, SYS_DUP, oldfd);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
dup2(int oldfd, int newfd)
{
    int ret = _SYSCALL2(int, SYS_DUP2, oldfd, newfd);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

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
    int ret = _SYSCALL3(int, SYS_EXECVE, name, argv, env);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
fchmod(int file, mode_t mode)
{
    int ret = _SYSCALL2(int, SYS_FCHMOD, file, mode);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
fchown(int file, uid_t owner, gid_t group)
{
    int ret = _SYSCALL3(int, SYS_FCHOWN, file, owner, group);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
fcntl(int fd, int cmd, ...)
{
    if (cmd != F_GETFD && cmd != F_SETFD) {
        return -1;
    }

    va_list argp;
    va_start(argp, cmd);
    void *arg = va_arg(argp, void*);
    va_end(argp);

    int ret = _SYSCALL3(int, SYS_FCNTL, fd, cmd, arg);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return 0;
}

int
fork()
{
    int ret = _SYSCALL0(int, SYS_FORK);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
fpathconf(int file, int name)
{
    return 0;
}

int
fstat(int file, struct stat *st)
{
    int ret = _SYSCALL2(int, SYS_FSTAT, file, st);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
ftruncate(int file, off_t length)
{
    int ret = _SYSCALL2(int, SYS_FTRUNCATE, file, length);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

char *
getcwd(char *buf, size_t size)
{
    int ret = _SYSCALL2(int, SYS_GETCWD, buf, size);

    if (ret < 0) {
        errno = -ret;
        return NULL;
    }

    return buf;
}

char *
getwd(char *buf)
{
    return getcwd(buf, 256);
}

int
getegid()
{
    int ret = _SYSCALL0(int, SYS_GETEGID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
geteuid()
{
    int ret = _SYSCALL0(int, SYS_GETEUID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getgid()
{
    int ret = _SYSCALL0(int, SYS_GETGID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

char *  
getlogin(void) 
{   
    static char login_buf[512];
    
    uid_t uid = getuid();

    struct passwd *pwd = getpwuid(uid);
    
    if (pwd) {
        strcpy(login_buf, pwd->pw_name, 512);
    
        return login_buf;
    }

    return "unknown";
}

int
getpgrp(pid_t pid)
{
    int ret = _SYSCALL1(int, SYS_GETPGRP, pid);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getpid()
{
    int ret = _SYSCALL0(int, SYS_GETPID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getppid()
{
    int ret = _SYSCALL0(int, SYS_GETPPID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
getsid()
{
    int ret = _SYSCALL0(int, SYS_GETSID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
gettid()
{
    int ret = _SYSCALL0(int, SYS_GETTID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
gettimeofday(struct timeval *p, void *z)
{
    int ret = _SYSCALL1(int, SYS_TIME, 0);

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
    int ret = _SYSCALL0(int, SYS_GETUID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
ioctl(int fd, unsigned long request, void *arg)
{
    int ret = _SYSCALL3(int, SYS_IOCTL, fd, (uint32_t)request, arg);

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
    int ret = _SYSCALL2(int, SYS_KILL, pid, sig);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
link(char *old, char *new)
{
    return -1;
}

int
lseek(int file, int ptr, int dir)
{
    int ret = _SYSCALL3(int, SYS_LSEEK, file, ptr, dir);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
lstat(const char *file, struct stat *st)
{
    int ret = _SYSCALL2(int, SYS_STAT, file, st);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
mkdir(const char *path, mode_t mode)
{
    int ret = _SYSCALL2(int, SYS_MKDIR, path, mode);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
mknod(const char *path, mode_t mode, dev_t dev)
{
    int ret = _SYSCALL3(int, SYS_MKNOD, path, mode, dev);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
mkfifo(const char *path, mode_t mode)
{
    return mknod(path, mode | S_IFIFO, 0);
}

int
mkpty()
{
    int ret = _SYSCALL0(int, SYS_MKPTY);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

void *
mmap(void *addr, size_t length, int prot, int flags,
        int fd, off_t offset)
{
    struct mmap_args args;

    args.addr = addr;
    args.length = length;
    args.prot = prot;
    args.flags = flags;
    args.fd = fd;
    args.offset = offset;

    int ret = _SYSCALL1(int, SYS_MMAP, &args);

    if (ret < 0) {
        errno = -ret;
        return NULL;
    }

    return (void*)ret;
}

unsigned int
msleep(unsigned int miliseconds)
{
    _SYSCALL1(void, SYS_SLEEP, miliseconds);

    return 0;
}

int
open(const char *name, int flags, ...)
{
    int ret = _SYSCALL2(int, SYS_OPEN, name, flags);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

long
pathconf(char *path, int name)
{
    return 0;
}

int
pause()
{
    int ret = _SYSCALL0(int, SYS_PAUSE);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
pipe(int pipefd[2])
{
    int ret = _SYSCALL1(int, SYS_PIPE, pipefd);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
read(int file, char *ptr, int len)
{
    int ret = _SYSCALL3(int, SYS_READ, file, ptr, len);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
rmdir(const char *path)
{
    int ret = _SYSCALL1(int, SYS_RMDIR, path);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

caddr_t
sbrk(int incr)
{
    int ret =  _SYSCALL1(int, SYS_SBRK, incr);
    
    if ((int)ret < 0) {
        errno = -ret;
        return -1;
    }

    return (caddr_t)ret;
}

int
setegid(gid_t gid)
{
    int ret = _SYSCALL1(int, SYS_SETEGID, gid);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
seteuid(uid_t uid)
{
    int ret = _SYSCALL1(int, SYS_SETEUID, uid);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
setgid(gid_t gid)
{
    int ret = _SYSCALL1(int, SYS_SETGID, gid);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
setpgid(pid_t pid, pid_t pgid)
{
    int ret = _SYSCALL2(int, SYS_SETPGID, pid, pgid);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
setsid()
{
    int ret = _SYSCALL0(int, SYS_SETSID);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
setuid(uid_t uid)
{
    int ret = _SYSCALL1(int, SYS_SETUID, uid);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int 
shm_open(const char *path, int oflag, mode_t mode)
{       
    int ret = _SYSCALL3(int, SYS_SHM_OPEN, path, oflag, mode);

    if (ret < 0) {
        errno = -ret;
        return -1; 
    }
    
    return ret;
}   

int
shm_unlink(const char *path)
{
    int ret = _SYSCALL1(int, SYS_SHM_UNLINK, path);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
sigrestore()
{
    int ret = _SYSCALL0(int, SYS_SIGRESTORE);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

unsigned int
sleep(unsigned int seconds)
{   
    return msleep(seconds * 1000);
}

int
socket(int domain, int type, int protocol)
{
    int ret = _SYSCALL3(int, SYS_SOCKET, domain, type, protocol);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
stat(const char *file, struct stat *st)
{
    int ret = _SYSCALL2(int, SYS_STAT, file, st);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

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
sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
    struct sysctl_args args;
    args.name = name;
    args.namelen = namelen;
    args.oldp = oldp;
    args.oldlenp = oldlenp;
    args.newp = newp;
    args.newlen = newlen;

    int ret = _SYSCALL1(int, SYS_SYSCTL, &args);

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
    int ret = _SYSCALL2(int, SYS_TRUNCATE, path, length);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
ttyname_r(int fd, char *buf, size_t buflen)
{
    int ret = _SYSCALL3(int, SYS_TTYNAME, fd, buf, buflen);

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
    return _SYSCALL1(mode_t, SYS_UMASK, newmode);
}

int
unlink(char *path)
{
    int ret = _SYSCALL1(int, SYS_UNLINK, path);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
uname(struct utsname *buf)
{
    int ret = _SYSCALL1(int, SYS_UNAME, buf);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
utime(const char *filename, const struct utimbuf *times)
{
    return 0;
}

int
wait(int *status)
{
    int ret = _SYSCALL1(int, SYS_WAIT, status);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int
waitpid(pid_t pid, int *status)
{
    int ret = _SYSCALL2(int, SYS_WAITPID, pid, status);

    if (ret < 0) {
        errno = -ret;

        return -1;
    }

    return ret;
}

int
write(int file, char *ptr, int len)
{
    int ret = _SYSCALL3(int, SYS_WRITE, file, ptr, len);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}
