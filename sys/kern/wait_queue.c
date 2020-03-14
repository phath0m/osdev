/*
 * wait_queue.c 
 *
 * Basically condition variables, allows multiple threads to wait for an event
 * to occur from a different thread
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <ds/list.h>
#include <sys/proc.h>
#include <sys/wait.h>

void
wq_empty(struct wait_queue *queue)
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

    list_destroy(&queue->waiting_threads, false);
}

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

    list_destroy(&queue->waiting_threads, false);

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
