#ifndef _DS_LIST_H
#define _DS_LIST_H

#include <stdbool.h>

#define LIST_COUNT(l)   ((l)->count)

struct list_elem {
    void                *   val;
    struct list_elem    *   next;
    struct list_elem    *   prev;
};

struct list {
    struct list_elem    *   head;
    struct list_elem    *   tail;
    int                     count;
};

typedef void (*list_free_t) (void *, void *);
typedef struct list_elem *  list_iter_t;


#define LIST_DESTROY_DEFAULT_CB list_destroy_default_cb

bool            iter_next_elem(list_iter_t *, void **);
bool            iter_peek_elem(list_iter_t *, void **);
void            list_destroy(struct list *, list_free_t, void *);
void            list_destroy_default_cb(void *, void *);
void            list_fini(struct list *, list_free_t, void *);
struct list *   list_new();
void            list_append(struct list *, void *);
void            list_append_front(struct list *, void *);
void    *       list_first(struct list *);
void            list_get_iter(struct list *, list_iter_t *);
bool            list_remove(struct list *, void *, list_free_t, void *);

#define LIST_INIT(p) \
    (p).head = NULL; \
    (p).tail = NULL; \
    (p).count = 0;

#endif
