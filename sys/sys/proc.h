#ifndef SYS_PROC_H
#define SYS_PROC_H

#include <ds/list.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/wait.h>

#define SRUN    0x02
#define SSLEEP  0x03
#define SSTOP   0x04
#define SZOMB   0x05
#define SWAIT   0x06
#define SLOCK   0x07
#define SDEAD   0x08

typedef int (*kthread_entry_t)(void *state);

struct regs;
struct proc;

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
    struct vfs_node *   cwd;
    struct vfs_node *   root;
    struct vm_space *   address_space;
    struct wait_queue   waiters;
    struct pgrp *       group;
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
    struct regs *       regs;
    struct proc *       proc;
    struct vm_space *   address_space;
    uint8_t             state;
    uintptr_t           stack;
    uintptr_t           u_stack_bottom;
    uintptr_t           u_stack_top;
};

/*
 * Pointer to the currently running process, defined in architecture specific scheduler implementation 
 */
extern struct proc *current_proc;

/*
 * proc_leave_session
 * Removes a group from a session, freeing the session if empty
 */
void pgrp_leave_session(struct pgrp *group, struct session *session);

/*
 * allocates new process group
 */
struct pgrp *pgrp_new(struct proc *leader, struct session *session);

/*
 * proc_execve
 * Loads an executable into the current address space and invokes the main entry point
 */
int proc_execve(const char *path, const char **argv, const char **envp);

/*
 * proc_fork
 * Forks the current process
 */
int proc_fork(struct regs *regs);

/*
 * proc_getctty
 * Gets the controlling terminal name for a given process
 */
char *proc_getctty(struct proc *proc);

void proc_getcwd(struct proc *proc, char *buf, int bufsize);

/*
 * proc_destroy
 * Frees a proc struct, killing any running threads and freeing all memory
 */
void proc_destroy(struct proc *);

/*
 * Remove a process from a particular group; freeing the group if empty
 */
void proc_leave_group(struct proc *proc, struct pgrp *group);

/*
 * proc_getbypid
 * Gets a proc pointer from a specified PID
 */
struct proc *proc_getbypid(int pid);

/*
 * proc_getfile
 * Obtains a struct file pointer from a given file descriptor
 */
struct file * proc_getfile(int fildes);

/*
 * proc_getfildes
 * Returns a free file descriptor
 */
int proc_getfildes();

/*
 * proc_new
 * Creates a new proc struct
 */
struct proc *proc_new();

/*
 * proc_newfildes
 * Creates a new file descriptor from a given file
 */
int proc_newfildes(struct file *file);

/*
 * proc_dup
 * Duplicates a file descriptor
 */
int proc_dup(int oldfd);

/*
 * proc_dup2
 * Duplicates a file descriptor
 */
int proc_dup2(int oldfd, int newfd);

/*
 * session_new()
 * allocates a new session
 */
struct session *session_new(struct proc *leader);

uintptr_t sched_init_thread(struct vm_space *space, uintptr_t stack_start, kthread_entry_t entry, void *arg);

void thread_run(kthread_entry_t entrypoint, struct vm_space *space, void *arg);

void thread_yield();

void thread_schedule(int state, struct thread *thread);

void thread_destroy(struct thread *thread);

#endif
