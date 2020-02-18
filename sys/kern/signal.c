#include <ds/list.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/systm.h>


static void
kill_thread(struct thread *thread)
{
    if (thread->interrupt_in_progress) {
        thread->exit_requested = 1;
        thread->terminated = 1;
    } else {
        thread_schedule(SDEAD, thread);
    }
}

int
proc_actually_kill(struct proc *proc, int signal)
{
    proc->status = signal;
    proc->exited = true;

    //kill_thread(proc->thread);

    list_iter_t iter;

    list_get_iter(&proc->threads, &iter);

    struct thread *thread;

    while (iter_move_next(&iter, (void**)&thread)) {
        kill_thread(thread);
    }

    iter_close(&iter);
    
    return 0;
}

int
default_kill(struct proc *proc, int signal)
{
    switch (signal) {
        case 9:  /* SIGKILL*/
        case 11: /* SIGSEGV */
        case 15: /* SIGTERM */
            return proc_actually_kill(proc, signal);
        default:
            break;
    }
    
    return 0;
}

int
proc_kill(struct proc *proc, int sig)
{
    struct sighandler *handler = proc->sighandlers[sig];
    
    if (!handler) {
        default_kill(proc, sig);
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
        proc->thread->exit_requested = 1;
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
