#ifndef _COLLECTIONS_DICT_H
#define _COLLECTIONS_DICT_H

#include <stdbool.h>

#include "list.h"

#define DICT_HASH_SIZE   101

struct dict_entry {
    struct list values;
};

struct dict {
    struct dict_entry * entries[DICT_HASH_SIZE];
    struct list         keys;
};


void dict_destroy(struct dict *dict);
bool dict_get(struct dict *dict, const char *key, void **result);
void dict_get_keys(struct dict *dict, list_iter_t *iter);
void dict_set(struct dict *dict, const char *key, void *value);

#endif
