#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <collections/list.h>
#include "tokenizer.h"


static size_t
new_string_size(size_t strsize)
{
    return (strsize + 1024) & ~0x3FF;
}

static char *
strdyncat(char **ptr, size_t *bufsize, const char *substr, size_t substrlen)
{
    if (substrlen == 0) {
        return *ptr;
    }

    if (!*ptr) {
        *bufsize = new_string_size(substrlen);
        *ptr = calloc(1, *bufsize);
        strncat(*ptr, substr, substrlen);    
        return *ptr;
    }

    size_t oldstrlen = strlen(*ptr);
    size_t newstrlen = oldstrlen + substrlen;

    if (newstrlen >= *bufsize) {
        *bufsize = new_string_size(newstrlen);
        *ptr = realloc(*ptr, *bufsize);
    }

    strncat(*ptr, substr, substrlen);
    return *ptr;
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

struct token *
token_new(token_kind_t kind, const char *value, int size)
{
    struct token *tok = (struct token*)calloc(1, (sizeof(struct token) + size + 1));
    
    char *buf = (char*)((intptr_t)tok + sizeof(struct token));

    strncpy(buf, value, size);

    tok->value = buf;
    tok->kind = kind;

    return tok;
}

static inline bool
is_symbol_character(int ch)
{
    return isalnum(ch) || ch == '/' || ch == '_' || ch == '-' || ch == '.' || ch == ':' || ch == '=' || ch == '\"';
}


static void 
scan_string_literal(struct tokenizer *scanner, char **ptr, size_t *bufsize)
{
    read_char(scanner);
    int start = scanner->position;

    while (peek_char(scanner) != '\"') {
        read_char(scanner);
    }

    int size = scanner->position - start;
    read_char(scanner);
    strdyncat(ptr, bufsize, &scanner->text[start], size);
}

static struct token *
scan_symbol(struct tokenizer *scanner)
{
    int start = scanner->position;
    char *ptr = NULL;
    size_t bufsize;
     
    while (is_symbol_character(peek_char(scanner))) {
        char ch = peek_char(scanner);
      
        if (ch == '\"') {
            if (scanner->position != start) {
                strdyncat(&ptr, &bufsize, &scanner->text[start], scanner->position - start);
            }
            scan_string_literal(scanner, &ptr, &bufsize);
            start = scanner->position;
        } else {
            read_char(scanner);
        }
    }

    if (ptr == NULL || start != scanner->position) {
        strdyncat(&ptr, &bufsize, &scanner->text[start], scanner->position - start);
    }

    return token_new(TOKEN_SYMBOL, ptr, strlen(ptr));
}

static struct token *
scan_token(struct tokenizer *scanner)
{
    while (isspace(peek_char(scanner))) {
        read_char(scanner);
    }

    int ch = peek_char(scanner);

    switch (ch) {
        case '|':
            read_char(scanner);
            return token_new(TOKEN_PIPE, "|", 1);
        case '>':
            read_char(scanner);
            return token_new(TOKEN_FILE_WRITE, ">", 1);
        default:
            break;
    }

    if (is_symbol_character(ch)) {
        return scan_symbol(scanner);
    }

    return NULL;
}

void
tokenizer_init(struct tokenizer *scanner, const char *text)
{
    scanner->text = text;
    scanner->length = strlen(text);
    scanner->position = 0;
}

void
tokenizer_scan(struct tokenizer *scanner, struct list *tokens)
{
    for (;;) {
        
        struct token *tok = scan_token(scanner);

        if (tok) {
            list_append(tokens, tok);
        } else {
            return;
        }
    }
}
