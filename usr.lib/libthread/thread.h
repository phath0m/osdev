#ifndef _THREAD_H
#define _THREAD_H

#include <sys/types.h>

typedef struct {
    void *      stack;
    void *      arg;
    void *      (*start_routine) (void *);
} thread_t;

typedef struct {
    pid_t       waitee;
    int         signaled;
} thread_cond_t;

void thread_create(thread_t *thread, void *(*start_routine) (void *), void *arg);

typedef unsigned char thread_spinlock_t;

void thread_spin_lock(thread_spinlock_t volatile *lock);
void thread_spin_unlock(thread_spinlock_t volatile *lock);

int thread_signal(pid_t tid);
void thread_pause();

void thread_cond_init(thread_cond_t *cond);
void thread_cond_wait(thread_cond_t *cond);
void thread_cond_signal(thread_cond_t *cond);

#endif
