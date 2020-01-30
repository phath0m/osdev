#include <ds/list.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/i686/interrupt.h>

void
thread_call_sa_handler(struct thread *thread, struct sigcontext *ctx)
{
    extern struct vm_space *sched_curr_address_space;

    ctx->invoked = true;

    uintptr_t stackp = (uintptr_t)vm_share(sched_curr_address_space, thread->address_space, NULL, (void*)(thread->stack & 0xFFFFF000), 0x1000, PROT_READ | PROT_WRITE | PROT_KERN);

    struct regs *regs = (struct regs*)((stackp + (thread->stack & 0xFFF)) + 4);

    ctx->regs = calloc(1, sizeof(struct regs));

    memcpy(ctx->regs, regs, sizeof(struct regs));

    regs->eip = ctx->handler_func; 

    vm_unmap(sched_curr_address_space, (void*)stackp, 0x1000);
}

void
thread_restore_signal_state(struct sigcontext *ctx, struct regs *regs)
{
    memcpy(regs, ctx->regs, sizeof(struct regs));
    free(ctx->regs);
}
