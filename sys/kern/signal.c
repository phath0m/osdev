/*
 * signal.c
 *
 * Machine independent portion of the kernel's implementation of
 * UNIX signals.
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
proc_actually_kill(struct proc *proc, int status)
{
    list_iter_t iter;

    struct thread *thread;

    proc->status = status;
    proc->exited = true;

    list_get_iter(&proc->threads, &iter);

    while (iter_move_next(&iter, (void**)&thread)) {
        kill_thread(thread);
    }


    wq_pulse(&proc->waiters);

    iter_close(&iter);
    
    return 0;
}

int
default_kill(struct proc *proc, int signal)
{
    switch (signal) {
        case 2:  /* SIGINT */
        case 9:  /* SIGKILL */
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
    struct sigcontext *ctx;
    struct sighandler *handler;
    
    handler = proc->sighandlers[sig];
    
    if (!handler) {
        default_kill(proc, sig);
        return 0;
    }

    ctx = calloc(1, sizeof(struct sigcontext));

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
    struct sighandler *handler;
    
    handler = proc->sighandlers[sig];

    if (!handler) {
        handler = calloc(1, sizeof(struct sighandler));
        proc->sighandlers[sig] = handler;
    }
    
    handler->handler = sargs->handler;
    handler->arg = sargs->arg;
    
    return 0;    
}
