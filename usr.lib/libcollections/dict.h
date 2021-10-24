#ifndef _COLLECTIONS_DICT_H
#define _COLLECTIONS_DICT_H

#include <stdbool.h>
#include "list.h"

#define DICT_HASH_SIZE  128

struct dict {
    struct list     entries[DICT_HASH_SIZE];    /* the actual hashmap */
    struct list     values;
    int             count;
};

struct dict_kvp {
    char            key[128];
    void    *       val;
};

typedef void (*dict_free_t) (void *, void*);

void            dict_destroy(struct dict *, dict_free_t, void *);
void            dict_fini(struct dict *, dict_free_t, void *);
struct dict *   dict_new();
bool            dict_has_key(struct dict *, const char *);
bool            dict_get(struct dict *, const char *, void **);
void            dict_get_iter(struct dict *, list_iter_t *);
bool            dict_remove(struct dict *, const char *, dict_free_t, void *);
void            dict_set(struct dict *, const char *, void *);

#endif
