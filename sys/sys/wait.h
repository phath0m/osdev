#ifndef _ELYSIUM_SYS_WAIT_H
#define _ELYSIUM_SYS_WAIT_H

#include <ds/list.h>
#include <sys/types.h>

struct wait_queue {
    struct list waiting_threads;
    bool        signaled;
    int         wait_count;
};


void wq_empty(struct wait_queue *);
void wq_pulse(struct wait_queue *);
void wq_wait(struct wait_queue *);

#endif
