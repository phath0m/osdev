/*
 * sys/i686/kern/syscall.c
 * x86 system call implementation via interrupt 0x80
 */
#include <rtl/malloc.h>
#include <rtl/types.h>
#include <sys/interrupt.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/i686/interrupt.h>
// remove
#include <rtl/printf.h>

struct syscall *syscall_table[256];

int
register_syscall(int num, int argc, syscall_t handler)
{
    struct syscall *syscall = (struct syscall*)calloc(0, sizeof(struct syscall));

    syscall->num = num;
    syscall->argc = argc;
    syscall->handler = handler;
    
    syscall_table[num] = syscall;

    return 0;
}

int
syscall_handler(int inum, struct regs *regs)
{
    int syscall_num = regs->eax;

    struct syscall *syscall = syscall_table[syscall_num];

    if (syscall) {
        /* defined in sys/i686/kern/sched.c */
        extern struct thread *sched_curr_thread;

        uintptr_t arguments[16];

        arguments[0] = (uintptr_t)regs->ebx;
        arguments[1] = (uintptr_t)regs->ecx;
        arguments[2] = (uintptr_t)regs->edx;
        arguments[3] = (uintptr_t)regs->esi;
        arguments[4] = (uintptr_t)regs->edi;

        struct syscall_args args;
        args.args = (uintptr_t*)&arguments;
        args.state = regs;

        int res = syscall->handler(sched_curr_thread->proc, &args);

        regs->eax = res;

        return 0;
    }

    regs->eax = -1;

    return -1;
}

__attribute__((constructor)) static void
_init_syscall()
{
    register_intr_handler(0x80, syscall_handler);
}
