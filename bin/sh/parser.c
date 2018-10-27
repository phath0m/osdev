#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "parser.h"
#include "tokenizer.h"

// remove
#include <stdio.h>

static struct token *
peek_token(struct parser *parser)
{
    struct token *token;

    if (iter_peek(&parser->iter, (void**)&token)) {
        return token;
    }

    return NULL;
}

static struct token *
read_token(struct parser *parser)
{
    struct token *token;

    if (iter_move_next(&parser->iter, (void**)&token)) {
        return token;
    }

    return NULL;
}

void
ast_node_destroy(struct ast_node *node)
{
    list_iter_t iter;

    list_get_iter(&node->children, &iter);

    struct ast_node *child;

    while (iter_move_next(&iter, (void**)&child)) {
        ast_node_destroy(child);
    }

    list_destroy(&node->children, false);

    free(node);
}

static struct ast_node *
ast_node_new(ast_class_t node_class, void *value)
{
    struct ast_node *node = (struct ast_node*)calloc(1, sizeof(struct ast_node));

    node->node_class = node_class;
    node->value = value;

    return node;
}

void
parser_init(struct parser *parser, struct list *tokens)
{
    list_get_iter(tokens, &parser->iter);
}

struct ast_node *
parser_parse(struct parser *parser)
{
    struct token *command = read_token(parser);
    struct ast_node *root = ast_node_new(AST_COMMAND, (void*)command->value);
    
    while (peek_token(parser)) {
        struct token *token = read_token(parser);
        
        if (token) {
            struct ast_node *arg = ast_node_new(AST_ARGUMENT, (void*)token->value);
            list_append(&root->children, arg);
        }
    }

    return root;
}
