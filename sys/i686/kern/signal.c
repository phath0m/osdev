#include <ds/list.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/i686/interrupt.h>

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

void
thread_call_sa_handler(struct thread *thread, struct sigcontext *ctx)
{
    extern struct vm_space *sched_curr_address_space;

    ctx->invoked = true;

    uintptr_t stackp = (uintptr_t)vm_share(sched_curr_address_space, thread->address_space, NULL, (void*)(thread->stack & 0xFFFFF000), 0x1000, PROT_READ | PROT_WRITE | PROT_KERN);

    struct irq_regs *regs = (struct irq_regs*)((stackp + (thread->stack & 0xFFF)));

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

    regs->eip = ctx->handler_func; 
    regs->eax = ctx->signo;
    regs->ebx = ctx->arg;

    vm_unmap(sched_curr_address_space, (void*)stackp, 0x1000);
}

void
thread_restore_signal_state(struct sigcontext *ctx, struct regs *regs)
{
    memcpy(regs, ctx->regs, sizeof(struct regs));
    free(ctx->regs);
}
