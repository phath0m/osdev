#ifndef _SYS_SYSCALLS_H
#define _SYS_SYSCALLS_H

#include <sys/types.h>

#define SYS_READ            0x00
#define SYS_WRITE           0x01
#define SYS_OPEN            0x02
#define SYS_CLOSE           0x03
#define SYS_STAT            0x04
#define SYS_FSTAT           0x05
#define SYS_LSEEK           0x06
#define SYS_FCNTL           0x07
#define SYS_IOCTL           0x08
#define SYS_SBRK            0x09
#define SYS_ACCESS          0x0A
#define SYS_EXECVE          0x0B
#define SYS_FORK            0x0C
#define SYS_EXIT            0x0D
#define SYS_UNAME           0x0E
#define SYS_CHROOT          0x0F
#define SYS_WAITPID         0x10
#define SYS_WAIT            0x11
#define SYS_READDIR         0x12
#define SYS_CHDIR           0x13
#define SYS_PIPE            0x14
#define SYS_SOCKET          0x15
#define SYS_CONNECT         0x16
#define SYS_DUP             0x17
#define SYS_DUP2            0x18
#define SYS_MKDIR           0x19
#define SYS_RMDIR           0x1A
#define SYS_CREAT           0x1B
#define SYS_UNLINK          0x1C
#define SYS_UMASK           0x1D
#define SYS_FCHMOD          0x1E
#define SYS_CHMOD           0x1F
#define SYS_MKPTY           0x20
#define SYS_ISATTY          0x21
#define SYS_TTYNAME         0x22
#define SYS_TIME            0x23
#define SYS_PAUSE           0x24
#define SYS_SETPGID         0x25
#define SYS_GETPGRP         0x26
#define SYS_GETPID          0x27
#define SYS_GETPPID         0x28
#define SYS_GETGID          0x29
#define SYS_GETEGID         0x2A
#define SYS_GETUID          0x2B
#define SYS_GETEUID         0x2C
#define SYS_SETGID          0x2D
#define SYS_SETEGID         0x2E
#define SYS_SETUID          0x2F
#define SYS_SETEUID         0x30
#define SYS_SETSID          0x31
#define SYS_GETSID          0x32
#define SYS_CHOWN           0x33
#define SYS_FCHOWN          0x34
#define SYS_TRUNCATE        0x37
#define SYS_FTRUNCATE       0x38
#define SYS_SLEEP           0x39
#define SYS_GETCWD          0x3A
#define SYS_KILL            0x3B
#define SYS_SIGACTION       0x3C
#define SYS_SIGRESTORE      0x3D
#define SYS_MKNOD           0x3E
#define SYS_ACCEPT          0x3F
#define SYS_BIND            0x40
#define SYS_MMAP            0x41
#define SYS_CLONE           0x44
#define SYS_THREAD_SLEEP    0x45
#define SYS_THREAD_WAKE     0x46
#define SYS_GETTID          0x47
#define SYS_SHM_OPEN        0x48
#define SYS_SHM_UNLINK      0x49
#define SYS_SYSCTL          0x4A

struct mmap_args {
    uintptr_t   addr;
    size_t      length;
    int         prot;
    int         flags;
    int         fd;
    off_t       offset;
};

struct sysctl_args {
    int *       name;
    int         namelen;
    void *      oldp;
    size_t *    oldlenp;
    void *      newp;
    size_t      newlen;
};

static inline uintptr_t
syscall0(int syscall)
{
    uintptr_t ret;

    asm volatile("int $0x80" : "=a"(ret) : "A"(syscall));

    return ret;
}

static inline uintptr_t
syscall1(int syscall, uintptr_t arg1)
{   
    uintptr_t ret;
    
    asm volatile("int $0x80" : "=a"(ret) : "A"(syscall), "b"(arg1));
    
    return ret;
}

static inline uintptr_t
syscall2(int syscall, uintptr_t arg1, uintptr_t arg2)
{
    uintptr_t ret;

    asm volatile("int $0x80" : "=a"(ret) : "A"(syscall), "b"(arg1), "c"(arg2));
    
    return ret;
}

static inline uintptr_t
syscall3(int syscall, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    uintptr_t ret;

    asm volatile("int $0x80" : "=a"(ret) : "A"(syscall), "b"(arg1), "c"(arg2), "d"(arg3));
    
    return ret;
}

static inline uintptr_t
syscall4(int syscall, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    uintptr_t ret;

    asm volatile("int $0x80" : "=a"(ret) : "A"(syscall), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4));

    return ret;
}

#define _SYSCALL0(type, syscall) (type)(syscall0(syscall))
#define _SYSCALL1(type, syscall, arg) (type)(syscall1(syscall, (uintptr_t)arg))
#define _SYSCALL2(type, syscall, arg1, arg2) (type)(syscall2(syscall, (uintptr_t)arg1, (uintptr_t)arg2))
#define _SYSCALL3(type, syscall, arg1, arg2, arg3) (type)(syscall3(syscall, (uintptr_t)arg1, (uintptr_t)arg2, (uintptr_t)arg3))
#define _SYSCALL4(type, syscall, arg1, arg2, arg3, arg4) (type)(syscall4(syscall, (uintptr_t)arg1, (uintptr_t)arg2, (uintptr_t)arg3, (uintptr_t)arg4))

#endif
