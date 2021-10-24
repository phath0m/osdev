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

    if (iter_peek_elem(&parser->iter, (void**)&token)) {
        return token;
    }

    return NULL;
}

static struct token *
read_token(struct parser *parser)
{
    struct token *token;

    if (iter_next_elem(&parser->iter, (void**)&token)) {
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

static bool
match_token_value(struct parser *parser, token_kind_t kind, const char *val)
{
    struct token *token;
    
    token = peek_token(parser);

    return token && token->kind == kind && strcmp(token->value, val) == 0;
}

static void
ast_node_destroy_cb(void *val, void *statep)
{
    struct ast_node *root;

    root = val;

    list_fini(&root->children, ast_node_destroy_cb, NULL);
    free(val);
}

void
ast_node_destroy(struct ast_node *node)
{
    list_fini(&node->children, ast_node_destroy_cb, NULL);
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

static bool
parse_noop(struct parser *parser)
{
    while (match_token_kind(parser, TOKEN_SEMICOLON) || match_token_kind(parser, TOKEN_NEWLINE)) {
        read_token(parser);
    }

    return peek_token(parser) != NULL;
}

static struct ast_node *
parse_command(struct parser *parser)
{
    struct ast_node *root;
    struct token *command;

    if (!match_token_kind(parser, TOKEN_SYMBOL)) {
        fprintf(stderr, "unexpected end of file\n");
        return NULL;
    }

    command = read_token(parser);

    root = ast_node_new(AST_COMMAND, (void*)command->value);

    while (match_token_kind(parser, TOKEN_SYMBOL) && peek_token(parser)) {
        struct ast_node *arg;
        struct token *token;
        
        token = read_token(parser);

        if (token && token->value) {
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

    substr = NULL;
    token = peek_token(parser);

    if (token->value) substr = strstr(token->value, "=");

    if (!substr) {
        return parse_command(parser);
    }

    read_token(parser);

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

        if (!right) goto error;

        pipe = ast_node_new(AST_PIPE, NULL);
        
        list_append(&pipe->children, left);
        list_append(&pipe->children, right);

        left = pipe;
    }

    return left;

error:
    ast_node_destroy(left);

    return NULL;
}

static struct ast_node *
parse_logical_and(struct parser *parser)
{
    struct ast_node *left;

    left = parse_pipe(parser);

    while (match_token_kind(parser, TOKEN_LOGICAL_AND)) {
        struct ast_node *right;
        struct ast_node *root;

        read_token(parser);
        
        right = parse_pipe(parser);

        if (!right) goto error;

        root = ast_node_new(AST_LOGICAL_AND, NULL);
        
        list_append(&root->children, left);
        list_append(&root->children, right);

        left = root;
    }

    return left;

error:
    ast_node_destroy(left);

    return NULL;
}

static struct ast_node *
parse_logical_or(struct parser *parser)
{
    struct ast_node *left;

    left = parse_logical_and(parser);

    while (match_token_kind(parser, TOKEN_LOGICAL_OR)) {
        struct ast_node *right;
        struct ast_node *root;

        read_token(parser);
        
        right = parse_logical_and(parser);

        if (!right) goto error;

        root = ast_node_new(AST_LOGICAL_OR, NULL);
        
        list_append(&root->children, left);
        list_append(&root->children, right);

        left = root;
    }

    return left;

error:
    ast_node_destroy(left);

    return NULL;
}

static struct ast_node *
parse_file_redirect(struct parser *parser)
{
    struct ast_node *left;
    
    left = parse_logical_or(parser);

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

static struct ast_node *
parse_line(struct parser *parser)
{
    return parse_file_redirect(parser);
}

static struct ast_node * parse_statement(struct parser *);

static struct ast_node *
parse_block(struct parser *parser, const char *start_kw, const char *end_kw)
{
    struct ast_node *cmd_list;

    if (!match_token_value(parser, TOKEN_KEYWORD, start_kw)) {
        fprintf(stderr, "expected '%s'\n", start_kw);
        return NULL;
    }

    read_token(parser);

    cmd_list = ast_node_new(AST_COMMAND_LIST, NULL);

    while (parse_noop(parser) && !match_token_value(parser, TOKEN_KEYWORD, end_kw)) {
        struct ast_node *cmd;
        cmd = parse_line(parser);

        if (!cmd) goto error; 

        list_append(&cmd_list->children, cmd);
    }

    if (!read_token(parser)) {
        fprintf(stderr, "unexpected end of file\n");
        goto error;
    }

    return cmd_list;

error:
    ast_node_destroy(cmd_list);
    return NULL;
}


static struct ast_node *
parse_if_statement(struct parser *parser, const char *start_kw)
{
    struct ast_node *expr;
    struct ast_node *if_stmt;
    struct ast_node *stmt;
    struct ast_node *body;
    struct ast_node *else_body;

    else_body = NULL;

    if (!match_token_value(parser, TOKEN_KEYWORD, start_kw)) {
        fprintf(stderr, "expected '%s'\n", start_kw);
        return NULL;
    }

    read_token(parser);

    if_stmt = ast_node_new(AST_IF_STMT, NULL);

    expr = parse_line(parser);

    if (!expr) goto error;

    list_append(&if_stmt->children, expr);

    if (!parse_noop(parser)) {
        fprintf(stderr, "unexpected end of file\n");
        goto error;
    }

    body = ast_node_new(AST_COMMAND_LIST, NULL);

    list_append(&if_stmt->children, body);

    if (!match_token_value(parser, TOKEN_KEYWORD, "then")) {
        fprintf(stderr, "expected 'then'\n");
        goto error;
    }

    read_token(parser);

    while (parse_noop(parser) && !match_token_value(parser, TOKEN_KEYWORD, "fi")) {
        if (match_token_value(parser, TOKEN_KEYWORD, "else")) {
            else_body = parse_block(parser, "else", "fi");
            break;
        } else if (match_token_value(parser, TOKEN_KEYWORD, "elif")) {
            else_body = parse_if_statement(parser, "elif");
            break;
        } else {
            stmt = parse_statement(parser);

            if (stmt) list_append(&body->children, stmt);
            else goto error;
        }
    }

    /* 
     * The code responsible for parsing the else is now responsible for handling
     * 'fi' so we don't have to worry about it
     */
    if (!else_body) {
        read_token(parser);
        list_append(&if_stmt->children, ast_node_new(AST_EMPTY, NULL));
    } else {
        list_append(&if_stmt->children, else_body);
    }

    return if_stmt;

error:
    if (if_stmt) ast_node_destroy(if_stmt);

    return NULL;
}

static struct ast_node *
parse_while_statement(struct parser *parser)
{
    struct ast_node *expr;
    struct ast_node *while_stmt;
    struct ast_node *body;

    while_stmt = ast_node_new(AST_WHILE_STMT, NULL);

    read_token(parser);

    expr = parse_line(parser);

    if (!expr) goto error;

    list_append(&while_stmt->children, expr);

    if (!parse_noop(parser)) {
        fprintf(stderr, "unexpected end of file\n");
        goto error;
    }

    body = parse_block(parser, "do", "done");

    if (!body) goto error;

    list_append(&while_stmt->children, body);

    return while_stmt;

error:
    if (while_stmt) ast_node_destroy(while_stmt);

    return NULL;
}

struct ast_node *
parse_statement(struct parser *parser)
{
    if (match_token_value(parser, TOKEN_KEYWORD, "if")) {
        return parse_if_statement(parser, "if");
    } else if (match_token_value(parser, TOKEN_KEYWORD, "while")) {
        return parse_while_statement(parser);
    } else {
        return parse_line(parser);
    }
}

struct ast_node *
parser_parse(struct parser* parser)
{
    struct ast_node *root;
    struct ast_node *child;
    root = ast_node_new(AST_COMMAND_LIST, NULL);

    while (parse_noop(parser)) {
        child = parse_statement(parser); 

        if (!child) goto error;

        list_append(&root->children, child);
    }

    return root;

error:
    ast_node_destroy(root);

    return NULL;
}
