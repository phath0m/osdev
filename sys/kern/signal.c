#include <ds/list.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/systm.h>

int
proc_kill(struct proc *proc, int sig)
{
    struct sighandler *handler = proc->sighandlers[sig];
    
    if (!handler) {
        return 0;
    }

    struct sigcontext *ctx = calloc(1, sizeof(struct sigcontext));

    ctx->handler_func = handler->handler;
    ctx->arg = handler->arg;
    ctx->signo = sig;

    if (!proc->thread->interrupt_in_progress) {
        /* 
         * If this is true; the thread is in userspace and we
         * we can poke to the registers directly, allowing us
         * just to change the instruction pointer and push our
         * restorer function on the stack
         */
        thread_call_sa_handler(proc->thread, ctx);
        list_append(&proc->thread->signal_stack, ctx);
    } else {
        list_append(&proc->thread->pending_signals, ctx);
    }
    return 0;
}

int
proc_signal(struct proc *proc, int sig, struct signal_args *sargs)
{
    struct sighandler *handler = proc->sighandlers[sig];

    if (!handler) {
        handler = calloc(1, sizeof(struct sighandler));
        proc->sighandlers[sig] = handler;
    }
    
    handler->handler = sargs->handler;
    handler->arg = sargs->arg;
    
    return 0;    
}
