#ifndef _SYS_SIGNAL_H
#define _SYS_SIGNAL_H

#include <sys/types.h>

typedef unsigned int sigset_t;

struct sigaction {
    uintptr_t   sa_handler;
    uintptr_t   sa_sigaction;
    sigset_t    sa_mask;
    int         sa_flags;
    uintptr_t   sa_restorer;
};

struct signal_args {
    uintptr_t       handler;
    uintptr_t       arg;
};

struct sighandler {
    uintptr_t   handler;
    uintptr_t   arg;
    int         mask;
    int         flags;
};

struct regs;

struct sigcontext {
    bool            invoked;
    uintptr_t       handler_func;
    uintptr_t       restorer_func;
    uintptr_t       arg;
    int             signo;
    struct regs *   regs;
};

#endif
