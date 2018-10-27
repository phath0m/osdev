#include <rtl/dict.h>
#include <rtl/list.h>
#include <rtl/malloc.h>
#include <rtl/mutex.h>
#include <rtl/string.h>
#include <rtl/types.h>

// reminder... remove this
#include <rtl/printf.h>

struct key_value_pair {
    char            key[128];
    void *          value;
};

static inline uint32_t
dict_hash(const char *key)
{
    uint32_t hash;

    for (hash = 0; *key; key++) {
        hash = *key + 31 * hash;
    }

    return hash % DICT_HASH_SIZE;
}

void
dict_clear(struct dict *dict)
{
    for (int i = 0; i < DICT_HASH_SIZE; i++) {
        struct dict_entry *entry = dict->entries[i];

        if (entry) {
            list_destroy(&entry->values, true);
            free(entry);
        }
    }
    /* TODO: implement... */
}

bool
dict_get(struct dict *dict, const char *key, void **result)
{
    uint32_t hash = dict_hash(key);

    struct dict_entry *entry = dict->entries[hash];

    bool succ = false;

    struct key_value_pair *kvp;

    if (entry && LIST_SIZE(&entry->values) > 1) {
        list_iter_t iter;

        list_get_iter(&entry->values, &iter);

        while (iter_move_next(&iter, (void**)&kvp)) {
            if (strcmp(key, kvp->key) == 0) {
                *result = kvp->value;
                succ = true;
                break;
            }
        }

        iter_close(&iter);

    } else if (entry) {
        kvp = (struct key_value_pair*)LIST_FIRST(&entry->values);
        *result = kvp->value;
        
        succ = true;
    }

    return succ;
}

void
dict_get_keys(struct dict *dict, list_iter_t *iter)
{
    list_get_iter(&dict->keys, iter);
}

void
dict_set(struct dict *dict, const char *key, void *value)
{
    uint32_t hash = dict_hash(key);

    struct dict_entry *entry = dict->entries[hash];

    if (!entry) {
        entry = (struct dict_entry*)calloc(0, sizeof(struct dict_entry));
        dict->entries[hash] = entry;
    }

    struct key_value_pair *kvp = (struct key_value_pair*)malloc(sizeof(struct key_value_pair));
    
    strncpy(kvp->key, key, 128);

    kvp->value = value;
    list_append(&entry->values, kvp);
    list_append(&dict->keys, kvp->key);
}
