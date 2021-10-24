#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"

bool
iter_next_elem(list_iter_t *iterp, void **val)
{
    struct list_elem *elem = (struct list_elem*)*iterp;

    if (elem) {
        *val = elem->val;
        *iterp = (list_iter_t)elem->next;
    }

    return (elem != NULL);
}

bool
iter_peek_elem(list_iter_t *iterp, void **val)
{
    struct list_elem *elem = (struct list_elem*)*iterp;

    if (elem) {
        *val = elem->val;
    }

    return (elem != NULL);
}

void
list_destroy_default_cb(void *val, void *statep)
{
    free(val);
}

void
list_destroy(struct list *listp, list_free_t free_func, void *state)
{
    list_fini(listp, free_func, state);
    free(listp);
}

void
list_fini(struct list *listp, list_free_t free_func, void *state)
{
    struct list_elem *cur = listp->head;

    while (cur) {
        struct list_elem *prev = cur;

        if (free_func) {
            free_func(prev->val, state);
        }

        cur = cur->next;

        free(prev);
    }
}

struct list *
list_new()
{
    struct list *listp = calloc(sizeof(struct list), 1);

    return listp;
}

void
list_append(struct list *listp, void *val)
{
    struct list_elem *elem = calloc(sizeof(struct list_elem), 1);
    elem->val = val;
    
    if (!listp->head) {
        listp->head = elem;
        listp->tail = elem;
    } else {
        elem->prev = listp->tail;
        elem->prev->next = elem;
        listp->tail = elem;
    }

    listp->count++;
}

void
list_append_front(struct list *listp, void *val)
{
    struct list_elem *elem = calloc(sizeof(struct list_elem), 1);
    elem->val = val;
    elem->next = listp->head;

    if (!listp->head) {
        listp->head = elem;
        listp->tail = elem;
    } else {
        listp->head = elem;
    }
    
    listp->count++;
}

void *
list_first(struct list *listp)
{
    struct list_elem *cur = listp->head;

    return cur->val;
}

void
list_get_iter(struct list *listp, list_iter_t *iterp)
{
    *iterp = (list_iter_t)listp->head;
}

bool
list_remove(struct list *listp, void *val, list_free_t free_func, void *state)
{
    struct list_elem *cur = listp->head;

    while (cur) {
        if (cur->val == val) {
            if (cur->prev) {
                cur->prev->next = cur->next;
            }

            if (cur->next) {
                cur->next->prev = cur->prev;
            }

            if (cur == listp->head) {
                listp->head = cur->next;
            }

            if (cur == listp->tail) {
                listp->tail = cur->prev;
            }

            if (free_func) {
                free_func(cur->val, state);
            }

            free(cur);

            listp->count--;
            return true;
        }

        cur = cur->next;
    }

    return false;
}
