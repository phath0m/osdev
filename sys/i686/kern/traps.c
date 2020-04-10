/*
 * traps.c - Exception handling code
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
#include <sys/systm.h>

#define DIVIDE_BY_ZERO              0x00
#define DEBUG                       0x01
#define NON_MASKABLE_INTR           0x02
#define BREAKPOINT                  0x03
#define OVERFLOW                    0x04
#define BOUND_RANGE_EXCEEDED        0x05
#define INVALID_OPCODE              0x06
#define DEVICE_NOT_AVAIL            0x07
#define DOUBLE_FAULT                0x08
#define INVALID_TSS                 0x0A
#define SEGMENT_NOT_PRESENT         0x0B
#define STACK_SEGMENT_FAULT         0x0C
#define GENERAL_PROTECTION_FAULT    0x0D
#define PAGE_FAULT                  0x0E
#define FLOATING_POINT_EXCEPTION    0x10
#define ALIGNMENT_CHECK             0x11
#define MACHINE_CHECK               0x12
#define VIRTUALIZATION_EXCEPTION    0x14

static const char *exceptions[] = {
    "divide by zero",
    "debug",
    "non maskable interrupt",
    "break",
    "overflow",
    "bound range exceeded",
    "invalid opcode",
    "device not available",
    "double fault",
    "reserved",
    "invalid tss",
    "segment not present",
    "stack segment fault",
    "general protection fault",
    "page fault",
    "reserved",
    "alignment check",
    "machine check",
    NULL
};

struct stackframe {
    struct stackframe * prev;
    uintptr_t           eip;
};

static void print_stack(struct regs *regs, int max_fames);
static void print_regs(struct regs *regs);

static int
handle_generic_exception(int inum, struct regs *regs)
{
    bus_interrupts_off();

    printf("\n");
    print_regs(regs);
    printf("\n");
    asm volatile("hlt");
    panic("%s", exceptions[inum]);
    
    asm volatile("hlt");

    return 0;
}

static int
handle_page_fault(int inum, struct regs *regs)
{
    bus_interrupts_off();

    int err = regs->error_code;

    bool present = (err & 0x1);
    bool rw = (err & 0x02) != 0;
    bool user = (err & 0x04) != 0;

    uint32_t fault_addr;

    asm volatile("movl %%cr2, %%edx": "=d"(fault_addr));

    if (user) {
        /* send SIGSEGV to program */
        extern struct proc *current_proc;

        printf("%s[%d] segfault at %p ip: %p sp %p\n\r", current_proc->name,
                current_proc->pid, fault_addr, regs->eip, regs->uesp);

        proc_kill(current_proc, 11);

        return 0;
    }
    
    printf("\n\r");
    print_regs(regs);
    printf("\n\r");
    print_stack(regs, 6);

    if (!present) {
        printf("page not present\n\r");
    }

    if (present && rw) {
        printf("page read only\n\r");
    }

    printf("\n\r");
    
    panic("page fault at %p", fault_addr);
    bus_interrupts_off();
    asm volatile("hlt");
    
    return 0;
}
static void
print_stack(struct regs *regs, int max_fames)
{
    char name[256];
    uintptr_t offset;
    struct stackframe *frame = (struct stackframe*)regs->ebp;
 
    printf("Trace:\n\r");

    for (int i = 0; i < max_fames && frame; i++) {
        if (ksym_find_nearest(frame->eip, &offset, name, 256) == 0) {
            printf("    [0x%p] <%s+0x%x>\n\r", frame->eip, name, offset);
        } else {
            printf("    [0x%p]\n\r", frame->eip);
        }
    
        if ((uintptr_t)frame->prev < 0xC0000000) {
            break;
        }    
        frame = frame->prev;
    }
}

static void
print_regs(struct regs *regs)
{
    char name[256];
    uintptr_t offset;

    if (ksym_find_nearest(regs->eip, &offset, name, 256) == 0) {
        printf("EIP is at %s+0x%x\n\r", name, offset);
    }

    printf("eax: %p ebx: %p ecx: %p edx: %p\n\r", regs->eax, regs->ebx, regs->ecx, regs->ebx);
    printf("edi: %p esi: %p esp: %p eip: %p\n\r", regs->edi, regs->esi, regs->esp, regs->eip);
    printf(" cs: %p  ds: %p  ss: %p\n\r", regs->cs, regs->ds, regs->ss);
}

void
traps_init()
{
    for (int i = 0; i < 14; i++) {
        intr_register(i, handle_generic_exception);
    }

    intr_register(14, handle_page_fault);
}
