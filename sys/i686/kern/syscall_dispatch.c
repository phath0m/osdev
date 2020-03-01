/*
 * sys/i686/kern/syscall.c
 * x86 system call implementation via interrupt 0x80
 */
#include <machine/reg.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/syscall.h>
#include <sys/types.h>

struct syscall *syscall_table[256];

int
register_syscall(int num, int argc, syscall_t handler)
{
    struct syscall *syscall = (struct syscall*)calloc(1, sizeof(struct syscall));

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
        uintptr_t arguments[16];

        arguments[0] = (uintptr_t)regs->ebx;
        arguments[1] = (uintptr_t)regs->ecx;
        arguments[2] = (uintptr_t)regs->edx;
        arguments[3] = (uintptr_t)regs->esi;
        arguments[4] = (uintptr_t)regs->edi;

        struct syscall_args args;
        args.args = (uintptr_t*)&arguments;
        args.state = regs;

        extern struct thread *sched_curr_thread;
        int res = syscall->handler(sched_curr_thread, &args);

        regs->eax = res;

        return 0;
    }

    regs->eax = -1;

    return -1;
}

__attribute__((constructor))
void
_init_syscall_handler()
{
    bus_register_intr(0x80, syscall_handler);
}
