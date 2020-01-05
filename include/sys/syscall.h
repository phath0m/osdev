#ifndef SYS_SYSCALL_H
#define SYS_SYSCALL_H

#include <sys/types.h>
#include <sys/proc.h>

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
#define SYS_CHROOT      0x0F
#define SYS_WAITPID     0x10
#define SYS_WAIT        0x11
#define SYS_READDIR     0x12
#define SYS_CHDIR       0x13
#define SYS_PIPE        0x14
#define SYS_SOCKET      0x15
#define SYS_CONNECT     0x16
#define SYS_DUP         0x17
#define SYS_DUP2        0x18
#define SYS_MKDIR       0x19
#define SYS_RMDIR       0x1A
#define SYS_CREAT       0x1B

#define DEFINE_SYSCALL_PARAM(type, name, num, argp) type name = ((type)argp->args[num])
#define DECLARE_SYSCALL_PARAM(type, num, argp) (type)(argp->args[num])


#define TRACE_SYSCALL(name, fmt, ...) printf("pid=%d   %s(" fmt ")\n", current_proc->pid, name, ## __VA_ARGS__);

struct syscall_args {
    void *      state;
    uintptr_t * args;
};

typedef struct syscall_args * syscall_args_t;

typedef int (*syscall_t)(syscall_args_t argv);

struct syscall {
    uint8_t     num;
    uint16_t    argc;
    syscall_t   handler;
};

int register_syscall(int num, int argc, syscall_t handler);


#endif
