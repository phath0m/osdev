#include <rtl/list.h>
#include <rtl/malloc.h>
#include <rtl/mutex.h>
#include <rtl/types.h>

void
list_append(struct list *listp, void *ptr)
{
    struct list_elem *elem = (struct list_elem*)calloc(0, sizeof(struct list_elem));

    spinlock_lock(&listp->lock);

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
    
    spinlock_unlock(&listp->lock);
}

void
list_destroy(struct list *listp, bool free_children)
{
    spinlock_lock(&listp->lock);

    struct list_elem *cur = listp->head;

    while (cur) {
        cur = cur->next_elem;

        if (free_children) {
            free(cur->data);
        }

        if (cur->prev_elem) {
            free(cur->prev_elem);
        }
    }

    spinlock_unlock(&listp->lock);
}

void
list_get_iter(struct list *listp, list_iter_t *iterp)
{
    spinlock_lock(&listp->lock);

    iterp->listp = listp;
    iterp->current_item = listp->head;
    iterp->iteration = 0;
}

bool
list_remove_top(struct list *listp, void **item)
{
    bool res = false;

    spinlock_lock(&listp->lock);

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

    spinlock_unlock(&listp->lock);
    
    return res;
}

void
iter_close(list_iter_t *iterp)
{
    struct list *listp = iterp->listp;

    spinlock_unlock(&listp->lock);
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
        return false;
    }

    iterp->current_item = next;

    *item = next->data;
    
    iterp->iteration++;

    return true;
}
