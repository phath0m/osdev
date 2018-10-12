#ifndef SYS_PROC_H
#define SYS_PROC_H

#include <rtl/list.h>
#include <rtl/types.h>
#include <sys/proc.h>
#include <sys/vm.h>

#define SRUN    0x02
#define SSLEEP  0x03
#define SSTOP   0x04
#define SZOMB   0x05
#define SWAIT   0x06
#define SLOCK   0x07

typedef int (*kthread_entry_t)(void *state);

struct container {
    int     id;
};

struct proc {
    struct list         threads;
    struct container *  container;
    struct file *       files[4096];
    struct thread *     thread;
    struct vfs_node *   root;
    struct vm_space *   address_space;
    pid_t               pid;
    uintptr_t           base;
    uintptr_t           brk;
};

struct thread {
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
 * proc_destroy
 * Frees a proc struct, killing any running threads and freeing all memory
 */
void proc_destroy(struct proc *);

/*
 * proc_new
 * Creates a new proc struct
 */
struct proc *proc_new();

uintptr_t sched_init_thread(struct vm_space *space, uintptr_t stack_start, kthread_entry_t entry, void *arg);
void sched_run_kthread(kthread_entry_t entrypoint, struct vm_space *space, void *arg);

#endif
