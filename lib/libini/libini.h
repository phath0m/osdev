#ifndef _LIBINI_H
#define _LIBINI_H

#include <collections/list.h>

struct ini {
    struct list values;
    void *      text;
};

struct ini_key {
    char *    section;
    char *    key;
    char *    value;
};

int ini_open(struct ini *ini, const char *file);
char *ini_get(struct ini *ini, const char *keyname);
char *ini_section_get(struct ini *ini, const char *section, const char *keyname);
void ini_close(struct ini *ini);

#endif
