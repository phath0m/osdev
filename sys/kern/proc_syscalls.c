#include <string.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/thread.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/vm.h>
// remove me
#include <stdio.h>

static inline int
can_execute_file(const char *path)
{
    struct stat buf;

    struct file *file;

    if (fops_open(current_proc->root, &file, path, O_RDONLY) == 0) {

        fops_stat(file, &buf);

        fops_close(file);
        
        if ((buf.st_mode & S_IXUSR) && buf.st_uid == current_proc->creds.euid) {
            return 0;
        }

        if ((buf.st_mode & S_IXGRP) && buf.st_gid == current_proc->creds.egid) {
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

    int res = fops_open_r(current_proc->root, current_proc->cwd, &file, path, O_RDONLY);

    if (res == 0) {
        if (current_proc->cwd) {
            DEC_NODE_REF(current_proc->cwd);
        }

        current_proc->cwd = file->node;

        fops_close(file);
    }

    return res;
}

static int
sys_chroot(syscall_args_t args)
{
    DEFINE_SYSCALL_PARAM(const char *, path, 0, args);

    TRACE_SYSCALL("chroot", "%s", path);

    struct vfs_node *root;
    
    int res = vfs_get_node(current_proc->root, current_proc->cwd, &root, path);

    if (res == 0) {
        current_proc->root = root;
    }

    return res;
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

    int status = fops_open(current_proc->root, &fd, file, O_RDONLY);

    if (status == 0) {
        char interpreter[512];

        if (fops_read(fd, interpreter, 2) == 2 && !strncmp(interpreter, "#!", 2)) {
            fops_read(fd, interpreter, 512);
        
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

            fops_close(fd);

            exec_err = can_execute_file(file);

            if (exec_err != 0) {
                return exec_err;
            }
            return proc_execve(interpreter, (const char **)new_argv, envp);

        } else {
            fops_close(fd);
        }
    }

    return proc_execve(file, argv, envp);
}

static int
sys_exit(syscall_args_t args)
{
    DEFINE_SYSCALL_PARAM(int, status, 0, args);

    TRACE_SYSCALL("exit", "%d", status);

    current_proc->status = status;
    current_proc->exited = true;
    //current_proc->thread->state = SZOMB;

    wq_pulse(&current_proc->waiters);

    thread_schedule(SDEAD, current_proc->thread);

    for (; ;) {
        thread_yield();
    }

    return 0;
}

static int
sys_fork(syscall_args_t argv)
{
    TRACE_SYSCALL("fork", "void");

    return proc_fork((struct regs*)argv->state);
}

static int
sys_sbrk(syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(size_t, increment, 0, argv);

    TRACE_SYSCALL("sbrk", "%d", increment);

    struct proc *proc = current_proc;

    uintptr_t old_brk = proc->brk;

    struct vm_space *space = proc->thread->address_space;

    vm_map(space, (void*)proc->brk, increment, PROT_READ | PROT_WRITE);

    proc->brk += increment;

    return old_brk;
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
}
