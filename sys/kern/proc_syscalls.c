#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/vm.h>

static inline int
can_execute_file(const char *path)
{
    struct stat buf;

    struct file *file;

    if (vops_open(current_proc, &file, path, O_RDONLY) == 0) {

        vops_stat(file, &buf);

        vops_close(file);
        
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
sys_chdir(syscall_args_t args)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, args);

    TRACE_SYSCALL("chdir", "%s", path);

    struct file *file;

    int res = vops_open_r(current_proc, &file, path, O_RDONLY);

    if (res == 0) {
        if (current_proc->cwd) {
            VN_DEC_REF(current_proc->cwd);
        }

        current_proc->cwd = file->node;

        vops_close(file);
    }

    return res;
}

static int
sys_chroot(syscall_args_t args)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, args);

    TRACE_SYSCALL("chroot", "%s", path);

    struct vnode *root;
    
    int res = vn_open(current_proc->root, current_proc->cwd, &root, path);

    if (res == 0) {
        current_proc->root = root;
    }

    return res;
}

static int
sys_clone(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(void *, func, 0, argv);
    DEFINE_SYSCALL_PARAM(void *, stack, 1, argv);
    DEFINE_SYSCALL_PARAM(int, flags, 2, argv);
    DEFINE_SYSCALL_PARAM(void *, arg, 3, argv);

    TRACE_SYSCALL("clone", "0x%p, 0x%p, %d, 0x%p", func, stack, flags, arg);

    return proc_clone(func, stack, flags, arg);    
}


static int
sys_execve(syscall_args_t args)
{
    DEFINE_SYSCALL_PARAM(const char *, file, 0, args);
    DEFINE_SYSCALL_PARAM(const char **, argv, 1, args);
    DEFINE_SYSCALL_PARAM(const char **, envp, 2, args);

    TRACE_SYSCALL("execve", "%s, %p, %p", file, argv, envp);

    struct file *fd;

    int exec_err = can_execute_file(file);

    if (exec_err != 0) {
        return exec_err;
    }

    int status = vops_open(current_proc, &fd, file, O_RDONLY);

    if (status == 0) {
        char interpreter[512];

        if (vops_read(fd, interpreter, 2) == 2 && !strncmp(interpreter, "#!", 2)) {
            vops_read(fd, interpreter, 512);
        
            for (int i = 0; i < 512; i++) {
                if (interpreter[i] == '\n') {
                    interpreter[i] = 0;
                    break;
                }
            }

            char *new_argv[256];

            for (int i = 0; i < 255 && argv[i]; i++) {
                new_argv[i + 2] = (char*)argv[i];
            }

            new_argv[0] = interpreter;
            new_argv[1] = (char*)file;

            vops_close(fd);

            exec_err = can_execute_file(file);

            if (exec_err != 0) {
                return exec_err;
            }
            return proc_execve(interpreter, (const char **)new_argv, envp);

        } else {
            vops_close(fd);
        }
    }

    return proc_execve(file, argv, envp);
}

static int
sys_exit(syscall_args_t args)
{
    DEFINE_SYSCALL_PARAM(int, status, 0, args);

    TRACE_SYSCALL("exit", "%d", status);

    extern struct thread *sched_curr_thread;

    list_remove(&current_proc->threads, sched_curr_thread);

    if (LIST_SIZE(&current_proc->threads) == 0) {
        current_proc->status = status;
        current_proc->exited = true;
        wq_pulse(&current_proc->waiters);
    }
    
    thread_schedule(SDEAD, sched_curr_thread);

    for (; ;) {
        thread_yield();
    }

    return 0;
}

static int
sys_getcwd(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(char *, buf, 0, argv);
    DEFINE_SYSCALL_PARAM(size_t, size, 1, argv);

    TRACE_SYSCALL("getcwd", "0x%p, %d", buf, size);

    proc_getcwd(current_proc, buf, size);
    
    return 0;
}

static int
sys_getgid(syscall_args_t argv)
{
    TRACE_SYSCALL("getid", "void");

    return current_proc->creds.gid;
}

static int
sys_getegid(syscall_args_t argv)
{
    TRACE_SYSCALL("geteid", "void");

    return current_proc->creds.egid;
}

static int
sys_getsid(syscall_args_t argv)
{
    TRACE_SYSCALL("getsid", "void");

    return current_proc->group->session->sid;
}

static int
sys_getuid(syscall_args_t argv)
{
    TRACE_SYSCALL("getuid", "void");

    return current_proc->creds.uid;
}

static int
sys_geteuid(syscall_args_t argv)
{
    TRACE_SYSCALL("geteuid", "void");

    return current_proc->creds.euid;
}

static int
sys_getpgrp(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(pid_t, pid, 0, argv);

    TRACE_SYSCALL("getpgrp", "%d", pid);

    if (pid != 0) {
        return -1;
    }

    return current_proc->group->pgid;
}

static int
sys_getpid(syscall_args_t argv)
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
sys_gettid(syscall_args_t argv)
{
    TRACE_SYSCALL("gettid", "void");

    extern struct thread *sched_curr_thread;

    return sched_curr_thread->tid;
}

static int
sys_fork(syscall_args_t argv)
{
    TRACE_SYSCALL("fork", "void");

    return proc_fork((struct regs*)argv->state);
}

static int
sys_kill(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(pid_t, pid, 0, argv);
    DEFINE_SYSCALL_PARAM(int, sig, 1, argv);
        
    TRACE_SYSCALL("kill", "%d, %d", pid, sig);

    struct proc *proc = proc_getbypid(pid);

    if (!proc) {
        return -(ESRCH);
    }

    proc_kill(proc, sig);

    return 0;
}

static int
sys_setgid(syscall_args_t argv)
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
sys_setegid(syscall_args_t argv)
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
sys_setpgid(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(pid_t, pid, 0, argv);
    DEFINE_SYSCALL_PARAM(pid_t, pgid, 1, argv);

    TRACE_SYSCALL("setpgid", "%d, %d", pid, pgid);

    if (pid != 0 || pgid != 0) {
        return -1;
    }

    struct pgrp *old_group = current_proc->group;
    struct pgrp *new_group = pgrp_new(current_proc, old_group->session);

    proc_leave_group(current_proc, old_group);

    current_proc->group = new_group;

    return 0;
}

static int
sys_setsid(syscall_args_t argv)
{
    TRACE_SYSCALL("setsid", "(void)");
    
    struct pgrp *old_group = current_proc->group;

    if (old_group->pgid == current_proc->pid) {
        return -(EPERM);
    }

    struct session *new_session = session_new(current_proc);
    struct pgrp *new_group = pgrp_new(current_proc, new_session);

    proc_leave_group(current_proc, old_group);

    current_proc->group = new_group;

    return 0;
}

static int
sys_setuid(syscall_args_t argv)
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
sys_seteuid(syscall_args_t argv)
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
sys_sbrk(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(ssize_t, increment, 0, argv);

    TRACE_SYSCALL("sbrk", "%d", increment);

    struct proc *proc = current_proc;

    uintptr_t old_brk = proc->brk;

    struct vm_space *space = proc->thread->address_space;

    if (increment > 0) {
        vm_map(space, (void*)proc->brk, increment, PROT_READ | PROT_WRITE);
    }

    proc->brk += increment;

    return old_brk;
}

static int
sys_sigaction(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, sig, 0, argv);
    DEFINE_SYSCALL_PARAM(struct signal_args *, sargs, 1, argv);

    proc_signal(current_proc, sig, sargs);

    return 0;
}

static int
sys_sigrestore(syscall_args_t argv)
{
    struct thread *curr_thread = current_proc->thread;

    struct sigcontext *ctx;

    list_remove_front(&curr_thread->signal_stack, (void**)&ctx);

    thread_restore_signal_state(ctx, SYSCALL_REGS(argv));
    
    return 0;
}

static int
sys_sleep(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, seconds, 0, argv);

    TRACE_SYSCALL("sleep", "%d", seconds);

    extern uint32_t timer_ticks;
    uint32_t required_ticks = seconds;

    int wakeup_time = timer_ticks + required_ticks;

    asm volatile("sti");

    while (timer_ticks < wakeup_time) {
        thread_yield();
    }

    return 0;
}

static int
sys_wait(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int *, status, 0, argv);

    TRACE_SYSCALL("wait", "%p", status);

    struct proc *child = LIST_LAST(&current_proc->children);

    if (child) {

        if (/*child->thread->state != SDEAD && */!child->exited) {
            asm volatile("sti");

            wq_wait(&child->waiters);
        }
        if (status) {
            *status = child->status;
        }
    }
    return -1;
}

static int
sys_waitpid(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(pid_t, pid, 0, argv);
    DEFINE_SYSCALL_PARAM(int *, status, 1, argv);

    TRACE_SYSCALL("waitpid", "%d, %p", pid, status);

    list_iter_t iter;

    list_get_iter(&current_proc->children, &iter);

    struct proc *proc;
    struct proc *needle = NULL;

    while (iter_move_next(&iter, (void**)&proc)) {
        if (proc->pid == pid) {
            needle = proc;
        }
    }

    iter_close(&iter);

    if (!needle) {
        return -1;
    }

    if (/*needle->thread->state != SDEAD && */!needle->exited) {
        asm volatile("sti");

        wq_wait(&needle->waiters);
    }

    if (status) {
        *status = needle->status;
    }

    return 0;
}

static int
sys_dup(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, oldfd, 0, argv);

    TRACE_SYSCALL("dup", "%d", oldfd);

    return proc_dup(oldfd);
}

static int
sys_dup2(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(int, oldfd, 0, argv);
    DEFINE_SYSCALL_PARAM(int, newfd, 1, argv);

    TRACE_SYSCALL("dup2", "%d, %d", oldfd, newfd);

    return proc_dup2(oldfd, newfd);
}

static int
sys_umask(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(mode_t, mask, 0, argv);

    TRACE_SYSCALL("umask", "%d", mask);

    mode_t oldmask = current_proc->umask;

    current_proc->umask = ~mask;

    return oldmask;
}

static int
sys_pause(syscall_args_t argv)
{
    TRACE_SYSCALL("pause", "void");

    asm volatile("sti");

    thread_schedule(SSLEEP, current_proc->thread);

    return 0;
}

static int
sys_thread_sleep(syscall_args_t argv)
{
    TRACE_SYSCALL("thread_sleep", "void");

    extern struct thread *sched_curr_thread;

    thread_schedule(SSLEEP, sched_curr_thread);

    asm volatile("sti");

    thread_yield();

    return 0;
}

static int
sys_thread_wake(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(pid_t, tid, 0, argv);

    TRACE_SYSCALL("thread_wake", "void");

    list_iter_t iter;

    list_get_iter(&current_proc->threads, &iter);

    int ret = -(ESRCH);
    struct thread *thread;

    while (iter_move_next(&iter, (void**)&thread)) {
        if (thread->tid == tid) {
            thread_schedule(SRUN, current_proc->thread);
            ret = 0;
            break;
        }
    }

    iter_close(&iter);

    return ret;
}

__attribute__((constructor))
void
_init_proc_syscalls()
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
}
