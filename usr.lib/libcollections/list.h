#ifndef _COLLECTIONS_LIST_H
#define _COLLECTIONS_LIST_H

#include <stdbool.h>
#include <stddef.h>

#define LIST_FIRST(list) ((list)->head ? (list)->head->data : NULL)
#define LIST_LAST(list)  ((list)->tail ? (list)->tail->data : NULL)
#define LIST_SIZE(list)  ((list)->count)

struct list_elem {
    void *  data;
    void *  next_elem;
    void *  prev_elem;
};

struct list {
    struct list_elem *  head;
    struct list_elem *  tail;
    size_t              count;
};

typedef struct {
    struct list *       listp;
    struct list_elem *  current_item;
    int                 iteration;
} list_iter_t;


void list_append(struct list *listp, void *ptr);
void list_destroy(struct list *listp, bool free_children);
void list_get_iter(struct list *listp, list_iter_t *iterp);
bool list_remove(struct list *listp, void *item);
bool list_remove_front(struct list *listp, void **item);
bool list_remove_back(struct list *listp, void **item);

bool iter_move_next(list_iter_t *iterp, void **item);
bool iter_peek(list_iter_t *iterp, void **item);

#endif
