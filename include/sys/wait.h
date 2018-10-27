#ifndef SYS_WAIT_H
#define SYS_WAIT_H

#include <rtl/list.h>
#include <rtl/types.h>

struct wait_queue {
    struct list waiting_threads;
    bool        signaled;
    int         wait_count;
};

void wq_pulse(struct wait_queue *queue);
void wq_wait(struct wait_queue *queue);

#endif
