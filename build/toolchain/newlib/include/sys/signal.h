#ifndef _SYS_SIGNAL_H
#define _SYS_SIGNAL_H

#include "_ansi.h"
#include <sys/features.h>
#include <sys/types.h>

#define SIGHUP  1
#define SIGINT  2
#define SIGQUIT 3
#define SIGILL  4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGFPE  8
#define SIGKILL 9
#define SIGBUS  10
#define SIGSEGV 11
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTOP 17
#define SIGTSTP 18
#define SIGCONT 19
#define SIGTTIN 21
#define SIGTTOU 22

#define NSIG        32

#define SIG_BLOCK   1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3

typedef unsigned long sigset_t;

typedef void (*_sig_func_ptr)();

struct sigaction {
    void            (*sa_handler)(int);
    void            (*sa_sigaction)(int, void *, void *);
    unsigned int    sa_mask;
    int             sa_flags;
    int             (*sa_restorer)(void);
};

int kill(pid_t, int);
int killpg(pid_t, int);
int sigaction(int, const struct sigaction *, struct sigaction *);
int sigpending(sigset_t*);
int sigsuspend(const sigset_t*);
int sigpause(int);

#endif
