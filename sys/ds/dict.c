/*
 * dict.c - Hashmap dictionary implementation
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
#include <ds/dict.h>
#include <ds/list.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/string.h>
#include <sys/types.h>

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

int
dict_count(struct dict *dict)
{
    return LIST_SIZE(&dict->keys);
}

void
dict_clear_f(struct dict *dict, dict_free_t free_func)
{
    for (int i = 0; i < DICT_HASH_SIZE; i++) {
        struct dict_entry *entry = dict->entries[i];

        if (!entry) {
            continue;
        }
        
        list_iter_t iter;
        list_get_iter(&entry->values, &iter);

        struct key_value_pair *kvp;

        while (iter_move_next(&iter, (void**)&kvp)) {
            free_func(kvp->value);           
        }

        iter_close(&iter);

        list_destroy(&entry->values, true);
        free(entry); 
    }
}

bool
dict_get(struct dict *dict, const char *key, void **result)
{
    uint32_t hash = dict_hash(key);

    struct dict_entry *entry = dict->entries[hash];

    if (!entry) {
        return false;
    }

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

        return succ;
    } 
    
    kvp = (struct key_value_pair*)LIST_FIRST(&entry->values);

    if (strcmp(key, kvp->key) == 0) {
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

bool
dict_remove(struct dict *dict, const char *key)
{
    uint32_t hash = dict_hash(key);
/*
    struct dict_entry *entry = dict->entries[hash];

    bool succ = false;

    struct key_value_pair *kvp;

    if (entry && LIST_SIZE(&entry->values) > 1) {
        list_iter_t iter;

        list_get_iter(&entry->values, &iter);

        while (iter_move_next(&iter, (void**)&kvp)) {
            if (strcmp(key, kvp->key) == 0) {
                succ = true;
                break;
            }
        }

        iter_close(&iter);

    } else if (entry) {
        kvp = (struct key_value_pair*)LIST_FIRST(&entry->values);
        succ = (strcmp(key, kvp->key) == 0);
        
        if (succ) {
            dict->entries[hash] = NULL;
        }
    }

    if (succ) {
        list_remove(&entry->values, kvp);
        list_remove(&dict->keys, kvp->key);
        free(kvp);
        free(entry);   
    }
    
    return succ;
    */

    struct dict_entry *entry = dict->entries[hash];
    struct list *listp = &entry->values;

    if (LIST_SIZE(listp) == 0) {
        return false;
    }

    struct key_value_pair *match = NULL;
    struct key_value_pair *kvp;

    list_iter_t iter;
    list_get_iter(listp, &iter);

    while (iter_move_next(&iter, (void**)&kvp)) {
        if (strncmp(kvp->key, key, sizeof(kvp->key)) == 0) {
            match = kvp;
            break;
        }
    }

    iter_close(&iter);

    if (match) {
        list_remove(listp, match);
        list_remove(&dict->keys, match->key);

        free(match);

        return true;
    }

    return false;
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

