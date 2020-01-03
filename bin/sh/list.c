#include <stdbool.h>
#include <stdlib.h>
#include "list.h"

void
list_append(struct list *listp, void *ptr)
{
    struct list_elem *elem = (struct list_elem*)calloc(1, sizeof(struct list_elem));

    elem->data = ptr;
    elem->next_elem = NULL;
    elem->prev_elem = NULL;

    struct list_elem *tail = listp->tail;

    if (tail) {
        tail->next_elem = elem;
        elem->prev_elem = tail;
    } else {
        listp->head = elem;
    }

    listp->tail = elem;
    
    listp->count++; 
}

void
list_destroy(struct list *listp, bool free_children)
{
    struct list_elem *cur = listp->head;

    while (cur) {
        if (free_children) {
            free(cur->data);
        }

        if (cur->prev_elem) {
            free(cur->prev_elem);
        }

        cur = cur->next_elem;
    }
}

void
list_get_iter(struct list *listp, list_iter_t *iterp)
{
    iterp->listp = listp;
    iterp->current_item = listp->head;
    iterp->iteration = 0;
}

bool
list_remove_front(struct list *listp, void **item)
{
    bool res = false;

    struct list_elem *head = listp->head;

    if (head) {
        listp->head = head->next_elem;

        if (listp->head) {
            listp->head->prev_elem = NULL;
        } else {
            listp->tail = NULL;
        }

        if (item) {
            *item = head->data;
        }

        listp->count--;

        free(head);

        res = true;
    }
    return res;
}

bool
list_remove(struct list *listp, void *item)
{
    bool res = false;
    
    struct list_elem *iter = listp->head;
    struct list_elem *prev = NULL;

    while (iter) {
        struct list_elem *next = iter->next_elem;

        if (iter->data == item) {
            if (prev) {
                prev->next_elem = iter->next_elem;
            } else {
                listp->head = next;
            }

            if (next) {
                next->prev_elem = prev;
            } else if (prev) {
                listp->tail = prev;
            }

            listp->count--;
            res = true;
            break;
        }

        prev = iter;
        iter = next;
    }

    return res;
}

bool
list_remove_back(struct list *listp, void **item)
{
    bool res = false;

    struct list_elem *tail = listp->tail;

    if (tail) {
        listp->tail = tail->prev_elem;
    
        if (listp->tail) {
            listp->tail->next_elem = NULL;
        }

        if (listp->head == tail) {
            listp->head = NULL;
        }

        if (item) {
            *item = tail->data;
        }

        listp->count--;

        free(tail);

        res = true;
    }
    
    return res;
}

bool
iter_move_next(list_iter_t *iterp, void **item)
{
    if (!iterp->current_item) {
        return false;
    }

    if (iterp->iteration == 0) {
        /* Its the very first item, so don't move next this time */

        *item = iterp->current_item->data;

        iterp->iteration++;

        return true;
    }

    struct list_elem *next = iterp->current_item->next_elem;

    if (!next) {
        iterp->current_item = NULL;
        return false;
    }

    iterp->current_item = next;

    *item = next->data;
    
    iterp->iteration++;

    return true;
}

bool
iter_peek(list_iter_t *iterp, void **item)
{
    if (!iterp->current_item) {
        return false;
    }

    if (iterp->iteration == 0) {
        *item = iterp->current_item->data;
        return true;
    }

    struct list_elem *next = iterp->current_item->next_elem;

    if (!next) {
        return false;
    }

    *item = next->data;

    return true;
}
