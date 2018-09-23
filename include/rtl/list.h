#ifndef RTL_LIST_H
#define RTL_LIST_H

#include <rtl/mutex.h>
#include <rtl/types.h>

struct list_elem {
    void *  data;
    void *  next_elem;
    void *  prev_elem;
};

struct list {
    struct list_elem *  head;
    struct list_elem *  tail;
    size_t              count;
    spinlock_t          lock;
};

typedef struct {
    struct list *       listp;
    struct list_elem *  current_item;
    int                 iteration;
} list_iter_t;


void list_append(struct list *listp, void *ptr);
void list_destroy(struct list *listp, bool free_children);

void list_get_iter(struct list *listp, list_iter_t *iterp);
void iter_close(list_iter_t *iterp);
bool iter_move_next(list_iter_t *iterp, void **item);

#endif
