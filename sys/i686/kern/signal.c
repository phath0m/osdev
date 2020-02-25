#include <ds/list.h>
#include <machine/reg.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>

/*
 *
 */
struct irq_regs {
    uint32_t    extra1;
    uint32_t    extra2;
    uint32_t    extra3;
    uint32_t    ds;
    uint32_t    edi;
    uint32_t    esi;
    uint32_t    ebp;
    uint32_t    esp;
    uint32_t    ebx;
    uint32_t    edx;
    uint32_t    ecx;
    uint32_t    eax;
    uint32_t    eip;
    uint32_t    cs;
    uint32_t    eflags;
    uint32_t    uesp;
    uint32_t    ss;
};

static uintptr_t *
open_stack(struct thread *thread, uintptr_t esp)
{
    extern struct vm_space *sched_curr_address_space;

    uintptr_t stackp = (uintptr_t)vm_share(sched_curr_address_space,
            thread->address_space, NULL,
            (void*)(esp & 0xFFFFF000), 0x1000,
            PROT_READ | PROT_WRITE | PROT_KERN
    );
    
    stackp &= 0xFFFFF000;
    stackp |= (esp & 0xFFF);

    return (uintptr_t*)stackp;
}

void
thread_call_sa_handler(struct thread *thread, struct sigcontext *ctx)
{
    extern struct vm_space *sched_curr_address_space;

    ctx->invoked = true;

    struct irq_regs *regs = (struct irq_regs*)open_stack(thread, thread->stack);
    ctx->regs = calloc(1, sizeof(struct regs));

    ctx->regs->ds = regs->ds;
    ctx->regs->edi = regs->edi;
    ctx->regs->esi = regs->esi;
    ctx->regs->ebp = regs->ebp;
    ctx->regs->esp = regs->esp;
    ctx->regs->ebx = regs->ebx;
    ctx->regs->edx = regs->edx;
    ctx->regs->ecx = regs->ecx;
    ctx->regs->eax = regs->eax;
    ctx->regs->eip = regs->eip;
    ctx->regs->cs = regs->cs;
    ctx->regs->eflags = regs->eflags;
    ctx->regs->uesp = regs->uesp;
    ctx->regs->ss = regs->ss;

    /* 
     * so this is kind of a stupid way to pass arguments (fastcall)
     * but basically I was having issues trying to push arguments
     * onto the usermode stack so I opted for this approach instead
     *
     * this will probably just be temporary until I can be arsed to
     * implement sane signals
     */
    regs->eip = ctx->handler_func; 
    regs->ecx = ctx->signo;
    regs->edx = ctx->arg;

    vm_unmap(sched_curr_address_space, (void*)regs, 0x1000);
}

void
thread_restore_signal_state(struct sigcontext *ctx, struct regs *regs)
{
    memcpy(regs, ctx->regs, sizeof(struct regs));
    free(ctx->regs);
}
