#include <ds/list.h>
#include <sys/malloc.h>
#include <sys/sched.h>
#include <sys/timer.h>

struct list timer_list;

void
timer_new(timer_tick_t handler, uint32_t timeout, void *argp)
{
    struct timer *timer = calloc(1, sizeof(struct timer));
    timer->expired = false;
    timer->expires = sched_ticks + timeout;
    timer->handler = handler;
    timer->argp = argp;
    
    list_append(&timer_list, timer);
}

void
timer_expire(struct timer *timer)
{
    list_remove(&timer_list, timer);
    free(timer);
}

void
timer_renew(struct timer *timer, uint32_t new_timeout)
{
    timer->expired = false;
    timer->expires = sched_ticks + new_timeout;
}

void
timer_tick()
{
    list_iter_t iter;
    list_get_iter(&timer_list, &iter);

    struct timer *timer;

    while (iter_move_next(&iter, (void**)&timer)) {
        if (!timer->expired && timer->expires < sched_ticks) {
            timer->expired = true;
            timer->handler(timer, timer->argp);

            if (timer->expired) {
                timer_expire(timer);
            }
        }
    }

    iter_close(&iter);
}
