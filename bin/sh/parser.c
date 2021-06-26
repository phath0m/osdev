#include <stdlib.h>
#include <string.h>
#include <collections/list.h>
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

static bool
match_token_kind(struct parser *parser, token_kind_t kind)
{
    struct token *token;
    
    token = peek_token(parser);

    return token && token->kind == kind;
}

void
ast_node_destroy(struct ast_node *node)
{
    list_iter_t iter;

    struct ast_node *child;

    list_get_iter(&node->children, &iter);

    while (iter_move_next(&iter, (void**)&child)) {
        ast_node_destroy(child);
    }

    list_destroy(&node->children, false);
    free(node);
}

static struct ast_node *
ast_node_new(ast_class_t node_class, void *value)
{
    struct ast_node *node;
    
    node = calloc(1, sizeof(struct ast_node));

    node->node_class = node_class;
    node->value = value;

    return node;
}

void
parser_init(struct parser *parser, struct list *tokens)
{
    list_get_iter(tokens, &parser->iter);
}

static struct ast_node *
parse_command(struct parser *parser)
{
    struct ast_node *root;
    struct token *command;

    command = read_token(parser);
    root = ast_node_new(AST_COMMAND, (void*)command->value);
    
    while (match_token_kind(parser, TOKEN_SYMBOL)) {
        struct ast_node *arg;
        struct token *token;
        
        token = read_token(parser);

        if (token) {
            arg = ast_node_new(AST_ARGUMENT, (void*)token->value);
            list_append(&root->children, arg);
        }
    }

    return root;
}

static struct ast_node *
parse_assignment(struct parser *parser)
{
    char *substr;
    char *val;
    struct ast_node *root;
    struct ast_node *child;
    struct token *token;
    
    token = peek_token(parser);
    substr = strstr(token->value, "=");

    if (!substr) {
        return parse_command(parser);
    }
    
    *substr = 0;
    val = substr + 1;

    root = ast_node_new(AST_ASSIGNMENT, (char*)token->value);
    child = ast_node_new(AST_ARGUMENT, (void*)val);

    list_append(&root->children, child);

    return root;
}

static struct ast_node *
parse_pipe(struct parser *parser)
{
    struct ast_node *left;

    left = parse_assignment(parser);

    while (match_token_kind(parser, TOKEN_PIPE)) {
        struct ast_node *right;
        struct ast_node *pipe;

        read_token(parser);
        
        right = parse_assignment(parser);
        pipe = ast_node_new(AST_PIPE, NULL);
        
        list_append(&pipe->children, left);
        list_append(&pipe->children, right);

        left = pipe;
    }

    return left;
}

static struct ast_node *
parse_file_redirect(struct parser *parser)
{
    struct ast_node *left;
    
    left = parse_pipe(parser);

    if (match_token_kind(parser, TOKEN_FILE_WRITE)) {
        struct ast_node *right;
        struct ast_node *file_redirect;
        struct token *token;

        read_token(parser);
        
        token = read_token(parser);

        right = ast_node_new(AST_ARGUMENT, (void*)token->value);;
        file_redirect = ast_node_new(AST_FILE_WRITE, NULL);

        list_append(&file_redirect->children, left);
        list_append(&file_redirect->children, right);

        return file_redirect;
    }

    return left;
}

struct ast_node *
parser_parse(struct parser* parser)
{
    return parse_file_redirect(parser);   
}
