#ifndef PARSER_H
#define PARSER_H

#include <collections/list.h>

typedef enum {
    AST_COMMAND,
    AST_ARGUMENT,
    AST_PIPE,
    AST_FILE_WRITE,
    AST_ASSIGNMENT
} ast_class_t;

struct ast_node {
    struct list children;
    ast_class_t node_class;
    void *      value;
};

struct parser {
    list_iter_t     iter;
};


void                parser_init(struct parser *parser, struct list *tokens);
struct ast_node *   parser_parse(struct parser *parser);
void                ast_node_destroy(struct ast_node *node);

#endif
