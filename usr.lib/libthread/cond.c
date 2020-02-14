#include "thread.h"

extern int gettid();

void
thread_cond_init(thread_cond_t *cond)
{
    cond->waitee = 0;
    cond->signaled = 0;
}

void
thread_cond_wait(thread_cond_t *cond)
{
    cond->waitee = gettid();

    while (!cond->signaled) {
        thread_pause();
    }

    cond->signaled = 0;
    cond->waitee = 0;
}

void
thread_cond_signal(thread_cond_t *cond)
{

    cond->signaled = 1;

    if (cond->waitee != 0) {
        thread_signal(cond->waitee);
    }
}
