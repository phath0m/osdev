#ifndef _SYS_THREAD_H
#define _SYS_THREAD_H

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

uintptr_t sched_init_thread(struct vm_space *space, uintptr_t stack_start, kthread_entry_t entry, void *arg);

void thread_run(kthread_entry_t entrypoint, struct vm_space *space, void *arg);

void thread_yield();

void thread_schedule(int state, struct thread *thread);

void thread_destroy(struct thread *thread);

#endif
