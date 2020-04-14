/*
 * syscall_dispatch.c - Machine dependent portion of the syscall interface
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
#include <machine/reg.h>
#include <sys/interrupt.h>
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

void
syscall_init()
{
    swi_register(0x80, syscall_handler);
}
