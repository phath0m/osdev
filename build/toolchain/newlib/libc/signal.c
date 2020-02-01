#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/syscalls.h>
#include <sys/types.h>

struct signal_args {
    uintptr_t       handler;
    uintptr_t       arg;
};

static struct sigaction sigactions[NSIG];

/*
 * fastcall? You must think I'm some sort of savage
 */
__attribute__((fastcall))
static void
_sighandle(int signo, uintptr_t handler_ptr)
{
    void (*handler_func)(int) = (void (*)(int))handler_ptr;

    handler_func(signo);

    sigrestore();
}

int
sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    memcpy((void*)&sigactions[signum], act, sizeof(struct sigaction));

    struct signal_args sargs;

    sargs.handler = (uintptr_t)_sighandle;
    sargs.arg = (uintptr_t)act->sa_handler;

    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SIGACTION), "b"(signum), "c"(&sargs));

    if (ret < 0) {
        errno = -ret;
        return -1;
    }
}

_sig_func_ptr
signal(int signo, _sig_func_ptr handler)
{
    struct signal_args sargs;

    sargs.handler = (uintptr_t)_sighandle;
    sargs.arg = (uintptr_t)handler;

    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SIGACTION), "b"(signo), "c"(&sargs));

    if (ret < 0) {
        errno = -ret;
        return NULL;
    }

    return NULL;
}
