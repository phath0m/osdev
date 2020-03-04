#ifndef _SYS_PROC_H
#define _SYS_PROC_H

#include <ds/list.h>
#include <sys/types.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/vm.h>
#include <sys/wait.h>

/* thread states */
#define SRUN    0x02
#define SSLEEP  0x03
#define SSTOP   0x04
#define SZOMB   0x05
#define SWAIT   0x06
#define SLOCK   0x07
#define SDEAD   0x08

#define CLONE_VM        0x01
#define CLONE_FILES     0x02

/* retrieve the current session for a process */
#define PROC_GET_SESSION(s) ((s)->group->session)
#define PROC_SID(s) ((s)->group->session->sid)
#define PROC_PGID(s) ((s)->group->pgid)

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
    struct cdev *     ctty;
    pid_t               sid;
};

struct pgrp {
    struct list         members;
    struct session *    session;
    struct proc *       leader;
    pid_t               pgid;           
};

/*
 * A process basically acts as a container to confine one or more  threads. Unlike
 * other UNIX-like systems, here, a process does not represent a scheduled task!
 *
 * Threads are the unit the scheduler works with, not processes.
 */
struct proc {
    struct cred         creds;              /* credentials */
    struct list         threads;            /* list of threads running under this process */
    struct list         children;           /* child processes */
    struct file *       files[4096];        /* file descriptor table */
    struct proc *       parent;             /* parent process */
    struct thread *     thread;             /* main thread attached to this process */
    struct vnode *      cwd;                /* current working directory */
    struct vnode *      root;               /* root directory */
    struct vm_space *   address_space;      /* address space this process is running in*/
    struct wait_queue   waiters;
    struct pgrp *       group;              /* process group this process is a member of*/
    struct sighandler * sighandlers[64];
    mode_t              umask;
    char                name[256];
    time_t              start_time;         /* epoch time this process started at */
    uintptr_t           base;               /* base address */
    uintptr_t           brk;                /* heap break */
    pid_t               pid;
    int                 status;             /* process exit code */
    bool                exited;
};

/*
 * A thread represents an actual task.
 */
struct thread {
    struct list         joined_queues;
    struct list         pending_signals;        /* list of signals to jump to when we return from kernel space */
    struct list         signal_stack;           /* list of signals currently "in-progress" */
    pid_t               tid;                    /* thread ID */
    struct regs *       regs;
    struct proc *       proc;                   /* host process */
    struct vm_space *   address_space;          /* address space, links to host process*/
    uint8_t             exit_requested;         /* should this be interrupted?  */
    uint8_t             terminated;             /* has this thread been terminated */
    uint8_t             interrupt_in_progress;  /* is this thread currently in kernel space*/
    uint8_t             state;
    uintptr_t           stack;
    uintptr_t           stack_base;             /* bottom of kernel mode stack */
    uintptr_t           stack_top;              /* top of kernel mode stack */
    uintptr_t           u_stack_bottom;         /* bottom of usermode stack */
    uintptr_t           u_stack_top;            /* top of usermode stack */
};

static inline bool
thread_exit_requested()
{
    extern struct thread *sched_curr_thread;
     
    return sched_curr_thread->exit_requested;
}

/*
 * Pointer to the currently running process, defined in architecture specific scheduler implementation 
 */
extern struct proc *current_proc;
extern struct pool  proc_pool;
extern struct pool  thread_pool;

struct pgrp *   pgrp_find(pid_t pgid);
void            pgrp_leave_session(struct pgrp *group, struct session *session);
struct pgrp *   pgrp_new(struct proc *leader, struct session *session);

int             proc_execve(const char *path, const char **argv, const char **envp);
int             proc_fork(struct regs *regs);
int             proc_clone(void *func, void *stack, int flags, void *arg);
char *          proc_getctty(struct proc *proc);
void            proc_getcwd(struct proc *proc, char *buf, int bufsize);
pid_t           proc_get_new_pid();
void            proc_destroy(struct proc *);
int             proc_kill(struct proc *proc, int sig);
void            proc_leave_group(struct proc *proc, struct pgrp *group);
struct proc *   proc_find(int pid);
struct proc *   proc_new();
int             proc_signal(struct proc *proc, int sig, struct signal_args *sargs);

struct session *    session_new(struct proc *leader);

uintptr_t       sched_init_thread(  struct vm_space *space, uintptr_t stack_start,
                                    kthread_entry_t entry, void *arg);

void            thread_call_sa_handler(struct thread *thread, struct sigcontext *ctx);

void            thread_interrupt_enter(struct thread *thread, struct regs *regs);
void            thread_interrupt_leave(struct thread *thread, struct regs *regs);
void            thread_restore_signal_state(struct sigcontext *ctx, struct regs *regs);
void            thread_run(kthread_entry_t entrypoint, struct vm_space *space, void *arg);
void            thread_yield();
void            thread_schedule(int state, struct thread *thread);
void            thread_destroy(struct thread *thread);
struct thread * thread_new(struct vm_space *space);

#endif
