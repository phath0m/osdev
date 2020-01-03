#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "list.h"

typedef enum {
    TOKEN_SYMBOL = 0,
    TOKEN_PIPE = 1
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

struct token *token_new(token_kind_t kind, const char *value, int size);

void tokenizer_init(struct tokenizer *scanner, const char *text);
void tokenizer_scan(struct tokenizer *scanner, struct list *tokens);

#endif
