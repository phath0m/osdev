#ifndef SYS_PROC_H
#define SYS_PROC_H

#include <ds/list.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/vm.h>
#include <sys/wait.h>

#define SRUN    0x02
#define SSLEEP  0x03
#define SSTOP   0x04
#define SZOMB   0x05
#define SWAIT   0x06
#define SLOCK   0x07
#define SDEAD   0x08

#define CLONE_VM        0x01
#define CLONE_FILES     0x02

typedef int (*kthread_entry_t)(void *state);

struct regs;
struct proc;
struct sighandler;

struct cred {
    uid_t   uid;
    gid_t   gid;
    uid_t   euid;
    gid_t   egid;
};

struct session {
    struct list         groups;
    struct proc *       leader;
    pid_t               sid;
};

struct pgrp {
    struct list         members;
    struct session *    session;
    struct proc *       leader;
    pid_t               pgid;           
};

struct proc {
    struct cred         creds;
    struct list         threads;
    struct list         children;
    struct file *       files[4096];
    struct proc *       parent;
    struct thread *     thread;
    struct vnode *      cwd;
    struct vnode *      root;
    struct vm_space *   address_space;
    struct wait_queue   waiters;
    struct pgrp *       group;
    struct sighandler * sighandlers[64];
    mode_t              umask;
    char                name[256];
    time_t              start_time;
    uintptr_t           base;
    uintptr_t           brk;
    pid_t               pid;
    int                 status;
    bool                exited;
};

struct thread {
    struct list         joined_queues;
    struct list         pending_signals; /* list of signals to jump to when we return from kernel space */
    struct list         signal_stack; /* list of signals currently "in-progress" */
    struct regs *       regs;
    struct proc *       proc;
    struct vm_space *   address_space;
    uint8_t             interrupt_in_progress;
    uint8_t             state;
    uintptr_t           stack;
    uintptr_t           stack_base;
    uintptr_t           stack_top;
    uintptr_t           u_stack_bottom;
    uintptr_t           u_stack_top;
};

/*
 * Pointer to the currently running process, defined in architecture specific scheduler implementation 
 */
extern struct proc *current_proc;

void pgrp_leave_session(struct pgrp *group, struct session *session);

struct pgrp *pgrp_new(struct proc *leader, struct session *session);

int proc_execve(const char *path, const char **argv, const char **envp);

int proc_fork(struct regs *regs);

int proc_clone(void *func, void *stack, int flags, void *arg);

char *proc_getctty(struct proc *proc);

void proc_getcwd(struct proc *proc, char *buf, int bufsize);

void proc_destroy(struct proc *);

int proc_kill(struct proc *proc, int sig);

void proc_leave_group(struct proc *proc, struct pgrp *group);

struct proc *proc_getbypid(int pid);

struct file * proc_getfile(int fildes);

int proc_getfildes();

struct proc *proc_new();

int proc_newfildes(struct file *file);

int proc_signal(struct proc *proc, int sig, struct signal_args *sargs);

int proc_dup(int oldfd);

int proc_dup2(int oldfd, int newfd);

struct session *session_new(struct proc *leader);

uintptr_t sched_init_thread(struct vm_space *space, uintptr_t stack_start, kthread_entry_t entry, void *arg);

void thread_call_sa_handler(struct thread *thread, struct sigcontext *ctx);

void thread_interrupt_enter(struct thread *thread, struct regs *regs);
void thread_interrupt_leave(struct thread *thread, struct regs *regs);

void thread_restore_signal_state(struct sigcontext *ctx, struct regs *regs);

void thread_run(kthread_entry_t entrypoint, struct vm_space *space, void *arg);

void thread_run_s(kthread_entry_t entrypoint, struct vm_space *space, void *stack, void *arg);

void thread_yield();

void thread_schedule(int state, struct thread *thread);

void thread_destroy(struct thread *thread);

#endif
