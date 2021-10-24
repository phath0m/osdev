#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <collections/list.h>

typedef enum {
    TOKEN_SYMBOL = 0,
    TOKEN_PIPE = 1,
    TOKEN_FILE_WRITE = 2,
    TOKEN_SEMICOLON = 3,
    TOKEN_LOGICAL_OR = 4,
    TOKEN_LOGICAL_AND = 5,
    TOKEN_AND = 6,
    TOKEN_KEYWORD = 7,
    TOKEN_NEWLINE = 8
} token_kind_t;

struct token {
    const char *    value;
    token_kind_t    kind;
};

struct tokenizer {
    const char *    text;
    int             length;
    int             position;
};

struct token *  token_new(token_kind_t kind, const char *value, int size);
void            tokenizer_init(struct tokenizer *scanner, const char *text);
void            tokenizer_scan(struct tokenizer *scanner, struct list *tokens);

#endif
