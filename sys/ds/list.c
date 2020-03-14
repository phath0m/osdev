/*
 * list.c - Linked list implementation
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
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/types.h>

/*
 * tracker for kernel heap
 */
int list_elem_count = 0;

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

    list_elem_count++;

    spinlock_unlock(&listp->lock);
}

void
list_destroy(struct list *listp, bool free_children)
{
    spinlock_lock(&listp->lock);

    struct list_elem *cur = listp->head;

    while (cur) {
        if (free_children) {
            free(cur->data);
        }

        struct list_elem *next = cur->next_elem;
        free(cur);
        cur = next;
        list_elem_count--;
    }

    listp->count = 0;
    listp->head = NULL;
    listp->tail = NULL;

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

void *
list_peek_back(struct list *listp)
{
    void *ret = NULL;

    spinlock_lock(&listp->lock);

    struct list_elem *tail = listp->tail;

    if (tail) {
        ret = tail->data;
    }

    spinlock_unlock(&listp->lock);

    return ret;
}

bool
list_remove_front(struct list *listp, void **item)
{
    bool res = false;

    spinlock_lock(&listp->lock);

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

        list_elem_count--;
        free(head);

        res = true;
    }

    spinlock_unlock(&listp->lock);
    
    return res;
}

bool
list_remove(struct list *listp, void *item)
{
    bool res = false;

    spinlock_lock(&listp->lock);
    
    struct list_elem *iter = listp->head;
    struct list_elem *prev = NULL;

    while (iter) {
        struct list_elem *next = iter->next_elem;

        if (iter->data == item) {
            if (prev) {
                prev->next_elem = iter->next_elem;
            } else {
                listp->head = iter->next_elem;
            }

            if (iter->next_elem) {
                next->prev_elem = prev;
            } else if (iter == listp->tail) {
                listp->tail = prev;
            }

            free(iter);
            list_elem_count--;
            listp->count--;
            res = true;
            break;
        }

        prev = iter;
        iter = iter->next_elem;
    }

    spinlock_unlock(&listp->lock);
    
    return res;
}

bool
list_remove_back(struct list *listp, void **item)
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
        list_elem_count--;

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
