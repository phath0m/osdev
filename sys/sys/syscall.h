/*
 * syscall.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _ELYSIUM_SYS_SYSCALL_H
#define _ELYSIUM_SYS_SYSCALL_H

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/proc.h>

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
#define SYS_MUNMAP          0x42
#define SYS_UTIMES          0x43
#define SYS_CLONE           0x44
#define SYS_THREAD_SLEEP    0x45
#define SYS_THREAD_WAKE     0x46
#define SYS_GETTID          0x47
#define SYS_SHM_OPEN        0x48
#define SYS_SHM_UNLINK      0x49
#define SYS_SYSCTL          0x4A
#define SYS_ADJTIME         0x4B
#define SYS_MOUNT           0x4C
#define SYS_LSEEK64         0x4D
#define SYS_WORLDCTL        0x4E

#define DEFINE_SYSCALL_PARAM(type, name, num, argp) type name = ((type)argp->args[num])
#define DECLARE_SYSCALL_PARAM(type, num, argp) (type)(argp->args[num])

#ifdef ENABLE_SYSCALL_TRACE
#define TRACE_SYSCALL(name, fmt, ...)  printf("pid=%d   %s(" fmt ")\n\r", current_proc->pid, name, ## __VA_ARGS__);
#else
#define TRACE_SYSCALL(name, fmt, ...) ;
#endif

#define SYSCALL_REGS(s) ((struct regs*)s->state)

struct syscall_args {
    void *      state;
    uintptr_t * args;
};
    
struct sysctl_args {
    int *       name;
    int         namelen;
    void *      oldp;
    size_t *    oldlenp;
    void *      newp;
    size_t      newlen;
};

struct thread;

typedef struct syscall_args * syscall_args_t;
typedef int (*syscall_t)(struct thread *, syscall_args_t);

struct syscall {
    uint8_t     num;
    uint16_t    argc;
    syscall_t   handler;
};

int register_syscall(int, int, syscall_t);

static inline void
syscalls_init()
{
    extern void mem_syscalls_init();
    extern void misc_syscalls_init();
    extern void proc_syscalls_init();
    extern void pty_syscalls_init();
    extern void socket_syscalls_init();
    extern void vfs_syscalls_init();
    
    mem_syscalls_init();
    misc_syscalls_init();
    proc_syscalls_init();
    pty_syscalls_init();
    socket_syscalls_init();
    vfs_syscalls_init();
}

#endif
