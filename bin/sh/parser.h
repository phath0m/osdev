#ifndef PARSER_H
#define PARSER_H

#include <collections/list.h>

typedef enum {
    AST_COMMAND = 0,
    AST_ARGUMENT = 1,
    AST_PIPE = 2,
    AST_FILE_WRITE = 3,
    AST_ASSIGNMENT = 4,
    AST_COMMAND_LIST = 5,
    AST_LOGICAL_AND = 6,
    AST_LOGICAL_OR = 7,
    AST_BACKGROUND = 8,
    AST_IF_STMT = 9,
    AST_EMPTY = 10,
    AST_WHILE_STMT = 11
} ast_class_t;

struct ast_node {
    struct list children;
    ast_class_t node_class;
    void *      value;
};

struct parser {
    int             error;
    list_iter_t     iter;
};


void                parser_init(struct parser *parser, struct list *tokens);
struct ast_node *   parser_parse(struct parser *parser);
void                ast_node_destroy(struct ast_node *node);

#endif
