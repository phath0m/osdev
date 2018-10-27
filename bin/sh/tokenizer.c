#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "tokenizer.h"

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
token_new(const char *value, int size)
{
    struct token *tok = (struct token*)calloc(1, (sizeof(struct token) + size + 1));
    
    char *buf = (char*)((intptr_t)tok + sizeof(struct token));

    strncpy(buf, value, size);

    tok->value = buf;

    return tok;
}

static inline bool
is_symbol_character(int ch)
{
    return isalnum(ch) || ch == '/' || ch == '_' || ch == '-' || ch == '.' || ch == ':';
}

static struct token *
scan_symbol(struct tokenizer *scanner)
{
    int start = scanner->position;
    const char *value = &scanner->text[start];
     
    while (is_symbol_character(peek_char(scanner))) {
        read_char(scanner);
    }

    int size = scanner->position - start;

    return token_new(value, size);
}

static struct token *
scan_token(struct tokenizer *scanner)
{
    while (isspace(peek_char(scanner))) {
        read_char(scanner);
    }

    if (is_symbol_character(peek_char(scanner))) {
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
