/*
 * proc_syscalls.c
 * 
 * This file implements system calls related to process creation, destruction,
 * and manipulation.
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
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/interrupt.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/procdesc.h>
#include <sys/sched.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/world.h>

static inline int
can_execute_file(const char *path)
{
    struct stat buf;
    struct file *file;

    if (vfs_open(current_proc, &file, path, O_RDONLY) == 0) {
        FOP_STAT(file, &buf);
        file_close(file);
        
        if ((buf.st_mode & S_IXUSR) && buf.st_uid == current_proc->creds.euid) {
            return 0;
        }

        if ((buf.st_mode & S_IXGRP) && buf.st_gid == current_proc->creds.egid) {
            return 0;
        }

        if ((buf.st_mode & S_IXOTH)) {
            return 0;
        }

        return -(EACCES);
    }

    return -(ENOENT);
}

static int
sys_chdir(struct thread *th, syscall_args_t args)
{
    int res;
    struct file *file;
    struct vnode *newdir;

    DEFINE_SYSCALL_PARAM(const char *, path, 0, args);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("chdir", "%s", path);

    res = vfs_open_r(current_proc, &file, path, O_RDONLY);

    if (res == 0) {
        if (current_proc->cwd) {
            VN_DEC_REF(current_proc->cwd);
        }

        if (FOP_GETVN(file, &newdir) == 0) current_proc->cwd = newdir;

        file_close(file);
    }

    return res;
}

static int
sys_chroot(struct thread *th, syscall_args_t args)
{
    int res;

    struct vnode *root;

    DEFINE_SYSCALL_PARAM(const char *, path, 0, args);

    if (vm_access(th->address_space, path, 1, VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("chroot", "%s", path);

    res = vn_open(current_proc->root, current_proc->cwd, &root, path);

    if (res == 0) {
        current_proc->root = root;
    }

    return res;
}

static int
sys_clone(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(void *, func, 0, argv);
    DEFINE_SYSCALL_PARAM(void *, stack, 1, argv);
    DEFINE_SYSCALL_PARAM(int, flags, 2, argv);
    DEFINE_SYSCALL_PARAM(void *, arg, 3, argv);

    if (vm_access(th->address_space, stack, 1, VM_READ | VM_WRITE)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("clone", "0x%p, 0x%p, %d, 0x%p", func, stack, flags, arg);

    return proc_clone(func, stack, flags, arg);    
}


static int
sys_execve(struct thread *th, syscall_args_t args)
{
    int exec_err;
    int i;
    int status;
    struct file *fd;

    char interpreter[512];
    char *new_argv[256];

    DEFINE_SYSCALL_PARAM(const char *, file, 0, args);
    DEFINE_SYSCALL_PARAM(const char **, argv, 1, args);
    DEFINE_SYSCALL_PARAM(const char **, envp, 2, args);

    TRACE_SYSCALL("execve", "%s, %p, %p", file, argv, envp);

    exec_err = can_execute_file(file);

    if (exec_err != 0) {
        return exec_err;
    }

    status = vfs_open(current_proc, &fd, file, O_RDONLY);

    if (status == 0) {
        if (FOP_READ(fd, interpreter, 2) == 2 && !strncmp(interpreter, "#!", 2)) {
            FOP_READ(fd, interpreter, 512);
        
            for (i = 0; i < 512; i++) {
                if (interpreter[i] == '\n') {
                    interpreter[i] = 0;
                    break;
                }
            }

            for (i = 0; i < 255 && argv[i]; i++) {
                new_argv[i + 2] = (char*)argv[i];
            }

            new_argv[0] = interpreter;
            new_argv[1] = (char*)file;
            new_argv[2] = NULL;

            file_close(fd);

            exec_err = can_execute_file(file);

            if (exec_err != 0) {
                return exec_err;
            }
            return proc_execve(interpreter, (const char **)new_argv, envp);

        } else {
            file_close(fd);
        }
    }

    return proc_execve(file, argv, envp);
}

static int
sys_exit(struct thread *th, syscall_args_t args)
{
    extern struct thread *sched_curr_thread;

    DEFINE_SYSCALL_PARAM(int, status, 0, args);

    TRACE_SYSCALL("exit", "%d", status);

    list_remove(&current_proc->threads, sched_curr_thread);

    if (LIST_SIZE(&current_proc->threads) == 0) {
        current_proc->exited = true;
        wq_pulse(&current_proc->waiters);
    }

    current_proc->status = status;

    thread_schedule(SDEAD, sched_curr_thread);

    for (; ;) {
        thread_yield();
    }

    return 0;
}

static int
sys_getcwd(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(char *, buf, 0, argv);
    DEFINE_SYSCALL_PARAM(size_t, size, 1, argv);

    if (vm_access(th->address_space, buf, size, VM_WRITE)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("getcwd", "0x%p, %d", buf, size);

    proc_getcwd(current_proc, buf, size);
    
    return 0;
}

static int
sys_getgid(struct thread *th, syscall_args_t argv)
{
    TRACE_SYSCALL("getid", "void");

    return current_proc->creds.gid;
}

static int
sys_getegid(struct thread *th, syscall_args_t argv)
{
    TRACE_SYSCALL("geteid", "void");

    return current_proc->creds.egid;
}

static int
sys_getsid(struct thread *th, syscall_args_t argv)
{
    TRACE_SYSCALL("getsid", "void");

    return current_proc->group->session->sid;
}

static int
sys_getuid(struct thread *th, syscall_args_t argv)
{
    TRACE_SYSCALL("getuid", "void");

    return current_proc->creds.uid;
}

static int
sys_geteuid(struct thread *th, syscall_args_t argv)
{
    TRACE_SYSCALL("geteuid", "void");

    return current_proc->creds.euid;
}

static int
sys_getpgrp(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(pid_t, pid, 0, argv);

    TRACE_SYSCALL("getpgrp", "%d", pid);

    if (pid != 0) {
        return -1;
    }

    return current_proc->group->pgid;
}

static int
sys_getpid(struct thread *th, syscall_args_t argv)
{
    TRACE_SYSCALL("getpid", "void");

    return current_proc->pid;
}

static int
sys_getppid()
{
    TRACE_SYSCALL("getppid", "void");

    return current_proc->parent->pid;
}

static int
sys_gettid(struct thread *th, syscall_args_t argv)
{
    extern struct thread *sched_curr_thread;

    TRACE_SYSCALL("gettid", "void");

    return sched_curr_thread->tid;
}

static int
sys_fcntl(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, fd, 0, argv);
    DEFINE_SYSCALL_PARAM(int, cmd, 1, argv);
    DEFINE_SYSCALL_PARAM(void*, arg, 2, argv);

    TRACE_SYSCALL("fcntl", "%d, %d, 0x%p", fd, cmd, arg);

    return procdesc_fcntl(fd, cmd, arg);
}

static int
sys_fork(struct thread *th, syscall_args_t argv)
{
    TRACE_SYSCALL("fork", "void");

    return proc_fork((struct regs*)argv->state);
}

static int
sys_kill(struct thread *th, syscall_args_t argv)
{
    struct proc *proc;

    DEFINE_SYSCALL_PARAM(pid_t, pid, 0, argv);
    DEFINE_SYSCALL_PARAM(int, sig, 1, argv);
        
    TRACE_SYSCALL("kill", "%d, %d", pid, sig);

    proc = proc_find(pid);

    if (!proc) {
        return -(ESRCH);
    }

    proc_kill(proc, sig);

    return 0;
}

static int
sys_setgid(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(gid_t, gid, 0, argv);

    TRACE_SYSCALL("setgid", "%d", gid);

    if (current_proc->creds.uid == 0) {
        current_proc->creds.gid = gid;
        current_proc->creds.egid = gid;
        return 0;
    }

    return -(EPERM);
}

static int
sys_setegid(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(gid_t, gid, 0, argv);

    TRACE_SYSCALL("setegid", "%d", gid);

    if (current_proc->creds.uid == 0 || current_proc->creds.gid == gid) {
        current_proc->creds.egid = gid;
        return 0;
    }

    return -(EPERM);
}

static int
sys_setpgid(struct thread *th, syscall_args_t argv)
{
    struct pgrp *old_group;
    struct pgrp *new_group;

    DEFINE_SYSCALL_PARAM(pid_t, pid, 0, argv);
    DEFINE_SYSCALL_PARAM(pid_t, pgid, 1, argv);

    TRACE_SYSCALL("setpgid", "%d, %d", pid, pgid);

    if (pid != 0 || pgid != 0) {
        return -1;
    }

    old_group = current_proc->group;
    new_group = pgrp_new(current_proc, old_group->session);

    proc_leave_group(current_proc, old_group);

    current_proc->group = new_group;

    return 0;
}

static int
sys_setsid(struct thread *th, syscall_args_t argv)
{
    struct session *new_session;
    struct pgrp *new_group;
    struct pgrp *old_group;

    TRACE_SYSCALL("setsid", "(void)");
    
    old_group = current_proc->group;

    if (old_group->pgid == current_proc->pid) {
        return -(EPERM);
    }

    new_session = session_new(current_proc);
    new_group = pgrp_new(current_proc, new_session);

    proc_leave_group(current_proc, old_group);

    current_proc->group = new_group;

    return 0;
}

static int
sys_setuid(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(uid_t, uid, 0, argv);

    TRACE_SYSCALL("setuid", "%d", uid);

    if (current_proc->creds.euid == 0) {
        current_proc->creds.uid = uid;
        current_proc->creds.euid = uid;
        return -1;
    }

    return -(EPERM);
}

static int
sys_seteuid(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(uid_t, uid, 0, argv);

    TRACE_SYSCALL("seteuid", "%d", uid);

    if (current_proc->creds.euid == 0 || uid == current_proc->creds.uid) {
        current_proc->creds.euid = uid;
        return 0;
    }

    return -(EPERM);
}

static int
sys_sbrk(struct thread *th, syscall_args_t argv)
{
    uintptr_t old_brk;
    struct proc *proc;
    struct vm_space *space;

    DEFINE_SYSCALL_PARAM(ssize_t, increment, 0, argv);

    TRACE_SYSCALL("sbrk", "%d", increment);

    proc = th->proc;
    old_brk = proc->brk;
    space = th->address_space;

    if ((old_brk + increment) >= 0x20000000) {
        printf("kernel: pid %d out of memory\n\r", proc->pid);
        printf("base = 0x%p\n\r", proc->base);
        printf("brk  = 0x%p\n\r", proc->brk);
        printf("incr = 0x%p\n\r", increment); 
        return -(ENOMEM);
    }

    if (increment > 0) {
        vm_map(space, (void*)proc->brk, increment, VM_READ | VM_WRITE);
    }

    proc->brk += increment;

    return old_brk;
}

static int
sys_sigaction(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, sig, 0, argv);
    DEFINE_SYSCALL_PARAM(struct signal_args *, sargs, 1, argv);

    proc_signal(current_proc, sig, sargs);

    return 0;
}

static int
sys_sigrestore(struct thread *th, syscall_args_t argv)
{
    struct thread *curr_thread;
    struct sigcontext *ctx;

    curr_thread = current_proc->thread;

    list_remove_front(&curr_thread->signal_stack, (void**)&ctx);

    thread_restore_signal_state(ctx, SYSCALL_REGS(argv));
    
    return 0;
}

static int
sys_sleep(struct thread *th, syscall_args_t argv)
{
    int wakeup_time;
    uint32_t required_ticks;

    DEFINE_SYSCALL_PARAM(int, milliseconds, 0, argv);

    TRACE_SYSCALL("msleep", "%d", milliseconds);

    required_ticks = milliseconds;
    wakeup_time = sched_ticks + required_ticks;

    bus_interrupts_on();

    while (sched_ticks < wakeup_time) {
        thread_yield();
    }

    return 0;
}

static int
sys_wait(struct thread *th, syscall_args_t argv)
{
    struct proc *child;
    DEFINE_SYSCALL_PARAM(int *, status, 0, argv);

    if (status && vm_access(th->address_space, status, sizeof(int), VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("wait", "%p", status);

    child = LIST_LAST(&current_proc->children);

    if (child) {
        if (!child->exited) {
            bus_interrupts_on();
            wq_wait(&child->waiters);
        }
        if (status) {
            *status = child->status;
        }
    }
    return -1;
}

static int
sys_waitpid(struct thread *th, syscall_args_t argv)
{
    list_iter_t iter;
    struct proc *proc;
    struct proc *needle;

    DEFINE_SYSCALL_PARAM(pid_t, pid, 0, argv);
    DEFINE_SYSCALL_PARAM(int *, status, 1, argv);

    if (status && vm_access(th->address_space, status, sizeof(int), VM_READ)) {
        return -(EFAULT);
    }

    TRACE_SYSCALL("waitpid", "%d, %p", pid, status);

    list_get_iter(&current_proc->children, &iter);

    needle = NULL;

    while (iter_move_next(&iter, (void**)&proc)) {
        if (proc->pid == pid) {
            needle = proc;
        }
    }

    iter_close(&iter);

    if (!needle) {
        return -1;
    }

    if (!needle->exited) {
        bus_interrupts_on();
        wq_wait(&needle->waiters);
    }

    if (status) {
        *status = needle->status;
    }

    return 0;
}

static int
sys_worldctl(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, opcode, 0, argv);
    DEFINE_SYSCALL_PARAM(int, arg, 1, argv);

    TRACE_SYSCALL("worldctl", "%d, %d", opcode, arg);

    return kern_worldctl(current_proc, opcode, arg);
}

static int
sys_dup(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, oldfd, 0, argv);

    TRACE_SYSCALL("dup", "%d", oldfd);

    return procdesc_dup(oldfd);
}

static int
sys_dup2(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, oldfd, 0, argv);
    DEFINE_SYSCALL_PARAM(int, newfd, 1, argv);

    TRACE_SYSCALL("dup2", "%d, %d", oldfd, newfd);

    return procdesc_dup2(oldfd, newfd);
}

static int
sys_umask(struct thread *th, syscall_args_t argv)
{
    mode_t oldmask;

    DEFINE_SYSCALL_PARAM(mode_t, mask, 0, argv);

    TRACE_SYSCALL("umask", "%d", mask);

    oldmask = current_proc->umask;

    current_proc->umask = ~mask;

    return oldmask;
}

static int
sys_pause(struct thread *th, syscall_args_t argv)
{
    TRACE_SYSCALL("pause", "void");

    bus_interrupts_on();

    thread_schedule(SSLEEP, current_proc->thread);

    return 0;
}

static int
sys_thread_sleep(struct thread *th, syscall_args_t argv)
{
    extern struct thread *sched_curr_thread;

    TRACE_SYSCALL("thread_sleep", "void");

    thread_schedule(SSLEEP, sched_curr_thread);

    bus_interrupts_on();

    thread_yield();

    return 0;
}

static int
sys_thread_wake(struct thread *th, syscall_args_t argv)
{
    int ret;
    list_iter_t iter;
    struct thread *thread;

    DEFINE_SYSCALL_PARAM(pid_t, tid, 0, argv);

    TRACE_SYSCALL("thread_wake", "void");

    list_get_iter(&current_proc->threads, &iter);

    ret = -(ESRCH);

    while (iter_move_next(&iter, (void**)&thread)) {
        if (thread->tid == tid) {
            thread_schedule(SRUN, thread);
            ret = 0;
            break;
        }
    }

    iter_close(&iter);

    return ret;
}

void
proc_syscalls_init()
{
    register_syscall(SYS_CHDIR, 1, sys_chdir);
    register_syscall(SYS_CHROOT, 1, sys_chroot);
    register_syscall(SYS_EXIT, 1, sys_exit);
    register_syscall(SYS_EXECVE, 3, sys_execve);
    register_syscall(SYS_FORK, 0, sys_fork);
    register_syscall(SYS_SBRK, 1, sys_sbrk);
    register_syscall(SYS_WAIT, 1, sys_wait);
    register_syscall(SYS_WAITPID, 2, sys_waitpid);
    register_syscall(SYS_DUP, 1, sys_dup);
    register_syscall(SYS_DUP2, 2, sys_dup2);
    register_syscall(SYS_UMASK, 1, sys_umask);
    register_syscall(SYS_PAUSE, 0, sys_pause);
    register_syscall(SYS_SETPGID, 2, sys_setpgid);
    register_syscall(SYS_GETPGRP, 1, sys_getpgrp);
    register_syscall(SYS_GETPID, 0, sys_getpid);
    register_syscall(SYS_GETPPID, 0, sys_getppid);
    register_syscall(SYS_GETGID, 0, sys_getgid);
    register_syscall(SYS_GETEGID, 0, sys_getegid);
    register_syscall(SYS_GETUID, 0, sys_getuid);
    register_syscall(SYS_GETEUID, 0, sys_geteuid);
    register_syscall(SYS_SETGID, 1, sys_setuid);
    register_syscall(SYS_SETEGID, 1, sys_seteuid);
    register_syscall(SYS_SETUID, 1, sys_setgid);
    register_syscall(SYS_SETEUID, 1, sys_setegid);
    register_syscall(SYS_SETSID, 0, sys_setsid);
    register_syscall(SYS_GETSID, 0, sys_getsid);
    register_syscall(SYS_SLEEP, 1, sys_sleep);
    register_syscall(SYS_GETCWD, 2, sys_getcwd);
    register_syscall(SYS_KILL, 2, sys_kill);
    register_syscall(SYS_SIGACTION, 2, sys_sigaction);
    register_syscall(SYS_SIGRESTORE, 0, sys_sigrestore);
    register_syscall(SYS_CLONE, 4, sys_clone);
    register_syscall(SYS_THREAD_SLEEP, 0, sys_thread_sleep);
    register_syscall(SYS_THREAD_WAKE, 1, sys_thread_wake);
    register_syscall(SYS_GETTID, 0, sys_gettid);
    register_syscall(SYS_FCNTL, 3, sys_fcntl);
    register_syscall(SYS_WORLDCTL, 2, sys_worldctl);
}