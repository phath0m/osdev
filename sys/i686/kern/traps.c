#include <machine/reg.h>
#include <sys/bus.h>
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
    struct stackframe *frame = (struct stackframe*)regs->ebp;
 
    printf("Trace:\n\r");

    for (int i = 0; i < max_fames && frame; i++) {
        printf("    [0x%p]\n\r", frame->eip);
        frame = frame->prev;
    }
}

static void
print_regs(struct regs *regs)
{
    printf("eax: %p ebx: %p ecx: %p edx: %p\n\r", regs->eax, regs->ebx, regs->ecx, regs->ebx);
    printf("edi: %p esi: %p esp: %p eip: %p\n\r", regs->edi, regs->esi, regs->esp, regs->eip);
    printf(" cs: %p  ds: %p  ss: %p\n\r", regs->cs, regs->ds, regs->ss);
}

__attribute__((constructor))
void
_init_exceptions()
{
    for (int i = 0; i < 14; i++) {
        bus_register_intr(i, handle_generic_exception);
    }

    bus_register_intr(14, handle_page_fault);
}
