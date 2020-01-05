#ifndef RTL_DICT_H
#define RTL_DICT_H

#include <ds/list.h>
#include <sys/mutex.h>
#include <sys/types.h>

#define DICT_HASH_SIZE   101

struct dict_entry {
    struct list values;
};

struct dict {
    struct dict_entry * entries[DICT_HASH_SIZE];
    struct list         keys;
    spinlock_t          lock;
};


void dict_clear(struct dict *dict);
int dict_count(struct dict *dict);
bool dict_get(struct dict *dict, const char *key, void **result);
void dict_get_keys(struct dict *dict, list_iter_t *iter);
bool dict_remove(struct dict *dict, const char *key);
void dict_set(struct dict *dict, const char *key, void *value);

#endif
