#include <rtl/types.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/vm.h>
// remove me
#include <rtl/printf.h>

uintptr_t
proc_sbrk(struct proc *proc, size_t increment)
{
    uintptr_t old_brk = proc->brk;

    struct vm_space *space = proc->thread->address_space;

    vm_map(space, (void*)proc->brk, increment, PROT_READ | PROT_WRITE);

    proc->brk += increment;

    return old_brk;
}

static int
sys_execve(struct proc *proc, syscall_args_t args)
{
    DEFINE_SYSCALL_PARAM(const char *, file, 0, args);
    DEFINE_SYSCALL_PARAM(const char **, argv, 1, args);
    DEFINE_SYSCALL_PARAM(const char **, envp, 2, args);

    return proc_execve(file, argv, envp);
}

static int
sys_fork(struct proc *proc, syscall_args_t argv)
{
    return proc_fork((struct regs*)argv->state);
}

static int
sys_sbrk(struct proc *proc, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(size_t, increment, 0, argv);

    return proc_sbrk(proc, increment);
}

__attribute__((constructor)) static void
_init_syscalls()
{
    register_syscall(SYS_EXECVE, 3, sys_execve);
    register_syscall(SYS_FORK, 0, sys_fork);
    register_syscall(SYS_SBRK, 1, sys_sbrk);
}
