#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <collections/list.h>
#include <sys/stat.h>
#include "libini.h"

struct tokenizer {
    int     position;
    int     length;
    char *  current_section;
    char *  text;
};

static inline char *
tokenizer_position(struct tokenizer *scanner)
{
    return &scanner->text[scanner->position];
}

static inline void
tokenizer_break(struct tokenizer *scanner)
{
    scanner->text[scanner->position++] = '\0';
}

static inline int
peek_char(struct tokenizer *scanner)
{
    if (scanner->position >= scanner->length) {
        return -1;
    }

    return scanner->text[scanner->position];
}

static inline int
read_char(struct tokenizer *scanner)
{
    int ch = peek_char(scanner);

    if (ch >= 0) {
        scanner->position++;
    }

    return ch;
}

static void
skip_until_newline(struct tokenizer *scanner)
{
    while (peek_char(scanner) != -1 && peek_char(scanner) != '\n') {
        read_char(scanner);
    }

    if (peek_char(scanner) == '\n') {
        tokenizer_break(scanner);
    }
}

static bool
read_section_name(struct tokenizer *scanner)
{
    read_char(scanner);

    scanner->current_section = tokenizer_position(scanner);

    while (isalnum(peek_char(scanner)) && peek_char(scanner) != -1) {
        read_char(scanner);
    }

    if (peek_char(scanner) != ']') {
        return false;
    }

    tokenizer_break(scanner);

    skip_until_newline(scanner);

    return true;
}

static struct ini_key *
read_key_value(struct tokenizer *scanner)
{
    char *key = tokenizer_position(scanner);

    while (isalnum(peek_char(scanner))) {
        read_char(scanner);
    }

    if (peek_char(scanner) != '=') {
        return NULL;
    }

    tokenizer_break(scanner);

    char *value = tokenizer_position(scanner);

    skip_until_newline(scanner);

    struct ini_key *val = calloc(1, sizeof(struct ini_key));    

    val->key = key;
    val->value = value;
    val->section = scanner->current_section;

    return val;   
}

int
ini_open(struct ini *ini, const char *file)
{
    memset(ini, 0, sizeof(struct ini));

    struct stat statbuf;

    if (stat(file, &statbuf) != 0) {
        return -1;
    }

    FILE *fp = fopen(file, "r");

    if (!fp) {
        return -1;
    }

    int fsize = (int)statbuf.st_size;
    char *text = calloc(1, fsize + 1);
    fread(text, 1, fsize, fp);
    fclose(fp);

    struct tokenizer scanner;

    memset(&scanner, 0, sizeof(scanner));

    scanner.text = text;
    scanner.length = strlen(text);
    ini->text = text;

    int ret = 0;
    int ch;

    while ((ch = peek_char(&scanner)) != -1) {
        if (isalnum(ch)) {
            struct ini_key *key = read_key_value(&scanner); 
            
            if (!key) {
                ret = -1;
                break;
            }
            list_append(&ini->values, key);
        } else if (ch == '[') {
            if (!read_section_name(&scanner)) {
                ret = -1;
                break;
            }
        } else if (ch == ';' || ch == '#') {
            skip_until_newline(&scanner);
        } else if (isspace(ch) || ch == '\n') {
            skip_until_newline(&scanner);
        } else {
            ret = -1;
            break;
        }
    }

    if (ret != 0) {
        free(scanner.text);
        list_destroy(&ini->values, true);
    }

    return ret;
}

char *
ini_get(struct ini *ini, const char *keyname)
{
    return 0;
}

char *
ini_section_get(struct ini *ini, const char *section, const char *keyname)
{
    list_iter_t iter;
    list_get_iter(&ini->values, &iter);

    struct ini_key *key;

    while (iter_move_next(&iter, (void**)&key)) {

        if (!key->section || strcmp(key->section, section)) continue;

        if (strcmp(key->key, keyname) == 0 && strcmp(key->section, section) == 0) {
            return key->value;
        }
    }

    return NULL;
}

void
ini_close(struct ini *ini)
{
    free(ini->text);
    list_destroy(&ini->values, true);
}
