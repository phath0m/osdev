#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "list.h"

struct token {
    const char *    value;
};

struct tokenizer {
    const char *    text;
    int             length;
    int             position;
};

struct token *token_new(const char *value, int size);

void tokenizer_init(struct tokenizer *scanner, const char *text);
void tokenizer_scan(struct tokenizer *scanner, struct list *tokens);

#endif
