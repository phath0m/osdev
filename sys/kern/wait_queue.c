#include <ds/list.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/wait.h>

// remove me
#include <stdio.h>

void
wq_pulse(struct wait_queue *queue)
{
    if (queue->wait_count == 0) {
        return;
    }

    list_iter_t iter;

    list_get_iter(&queue->waiting_threads, &iter);

    struct thread *thread;

    queue->signaled = true;

    while (iter_move_next(&iter, (void**)&thread)) {
        thread_schedule(SRUN, thread);
    }

    iter_close(&iter);
    
    while (queue->wait_count) {
        thread_yield();
    }

    queue->signaled = false;
}

void
wq_wait(struct wait_queue *queue)
{
    __sync_add_and_fetch(&queue->wait_count, 1);

    if (!queue->signaled) {
        list_append(&queue->waiting_threads, current_proc->thread);

        thread_schedule(SSLEEP, current_proc->thread);

        while (!queue->signaled) {
        }
    }
    __sync_add_and_fetch(&queue->wait_count, -1);
}
