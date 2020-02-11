#ifndef _THREAD_H
#define _THREAD_H

typedef struct {
    void *      stack;
    void *      arg;
    void *      (*start_routine) (void *);
} thread_t;


void thread_create(thread_t *thread, void *(*start_routine) (void *), void *arg);

typedef unsigned char thread_spinlock_t;

void thread_spin_lock(thread_spinlock_t volatile *lock);
void thread_spin_unlock(thread_spinlock_t volatile *lock);

#endif
