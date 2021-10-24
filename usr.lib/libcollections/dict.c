#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dict.h"
#include "list.h"


/* arguments to pass to function responsible for freeing each element inside the dictionary */
struct dict_destroy_state {
    dict_free_t func;
    void    *   state;
};

static inline uint32_t
dict_hash(const char *str)
{
    uint32_t res = 2166136261;

    while (*str) {
        res ^= *str;
        res *= 0x01000193;
        str++;
    }

    return res & 127;
}

static void
dict_destroy_func(void *p, void *s)
{
    struct dict_kvp *kvp = p;
    struct dict_destroy_state *state = s;

    if (state->func) {
        state->func(kvp->val, state->state);
    }

    free(kvp);
}

void
dict_destroy(struct dict *dictp, dict_free_t free_func, void *statep)
{
    dict_fini(dictp, free_func, statep);
    free(dictp);
}

void
dict_fini(struct dict *dictp, dict_free_t free_func, void *statep)
{
    struct dict_destroy_state state = {
        .func   =   free_func,
        .state  =   statep
    };

    for (int i = 0; i < DICT_HASH_SIZE; i++) {
        struct list *listp = &dictp->entries[i];

        if (LIST_COUNT(listp) > 0) {
            list_fini(listp, NULL, NULL);
        }
    }

    list_fini(&dictp->values, dict_destroy_func, &state);
}

struct dict *
dict_new()
{
    struct dict *dictp = calloc(sizeof(struct dict), 1);

    return dictp;
}

bool
dict_has_key(struct dict *dictp, const char *key)
{
    uint32_t hash = dict_hash(key);

    if (LIST_COUNT(&dictp->entries[hash]) == 0) {
        return false;
    }

    struct list *listp = &dictp->entries[hash];
    struct dict_kvp *kvp;
    list_iter_t iter;

    list_get_iter(listp, &iter);

    while (iter_next_elem(&iter, (void**)&kvp)) {
        if (strncmp(kvp->key, key, sizeof(kvp->key)) == 0) {
            return true;
        }
    }

    return false;
}

bool
dict_get(struct dict *dictp, const char *key, void **res)
{
    uint32_t hash = dict_hash(key);

    if (LIST_COUNT(&dictp->entries[hash]) == 0) {
        return false;
    }

    struct list *listp = &dictp->entries[hash];
    struct dict_kvp *kvp;
    list_iter_t iter;

    list_get_iter(listp, &iter);

    while (iter_next_elem(&iter, (void**)&kvp)) {
        if (strncmp(kvp->key, key, sizeof(kvp->key)) == 0) {
            *res = kvp->val;
            return true;
        }
    }

    return false;
}

void
dict_get_iter(struct dict *dictp, list_iter_t *iter)
{
    list_get_iter(&dictp->values, iter);
}

bool
dict_remove(struct dict *dictp, const char *key, dict_free_t free_func, void *state)
{
    uint32_t hash = dict_hash(key);

    if (LIST_COUNT(&dictp->entries[hash]) == 0) {
        return false;
    }

    struct list *listp = &dictp->entries[hash];
    struct dict_kvp *match = NULL;
    struct dict_kvp *kvp;
    list_iter_t iter;

    list_get_iter(listp, &iter);

    while (iter_next_elem(&iter, (void**)&kvp)) {
        if (strncmp(kvp->key, key, sizeof(kvp->key)) == 0) {
            match = kvp;
            break;
        }
    }

    if (match) {
        list_remove(listp, match, NULL, NULL);
        list_remove(&dictp->values, match, NULL, NULL);

        if (free_func) {
            free_func(match->val, state);
        }

        free(match);

        return true;
    }

    return false;
}

void
dict_set(struct dict *dictp, const char *key, void *val)
{
    uint32_t hash = dict_hash(key);

    struct list *listp = &dictp->entries[hash];
    struct dict_kvp *cur_kvp;
    list_iter_t iter;

    list_get_iter(listp, &iter);

    while (iter_next_elem(&iter, (void**)&cur_kvp)) {
        if (strncmp(cur_kvp->key, key, sizeof(cur_kvp->key)) == 0) {
            cur_kvp->val = val;
            return;
        }
    }

    struct dict_kvp *kvp = calloc(sizeof(struct dict_kvp), 1);
    kvp->val = val;
    strncpy(kvp->key, key, sizeof(kvp->key)-1);

    list_append(listp, kvp);
    list_append(&dictp->values, kvp);
}