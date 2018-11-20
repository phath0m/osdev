#include <ds/list.h>
#include <sys/proc.h>
#include <sys/wait.h>

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
        schedule_thread(SRUN, thread);
    }

    iter_close(&iter);
    
    while (queue->wait_count) {
        sched_yield();
    }
}

void
wq_wait(struct wait_queue *queue)
{
    queue->wait_count++;
    
    list_append(&queue->waiting_threads, current_proc->thread);

    schedule_thread(SSLEEP, current_proc->thread);

    while (!queue->signaled) {

    }

    queue->wait_count--;
}
