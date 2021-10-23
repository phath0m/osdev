#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct token_stream {
    int             pos;
    int             len;
    const char **   tokens;
};

typedef enum {
    AST_BINOP_EQ,
    AST_BINOP_GT,
    AST_BINOP_LT,
    AST_BINOP_GE,
    AST_BINOP_LE,
    AST_BINOP_ADD,
    AST_BINOP_SUB,
    AST_BINOP_MUL,
    AST_BINOP_DIV,
    AST_BINOP_AND,
    AST_BINOP_OR,
    AST_TERMINAL
} ast_kind_t;

struct ast_node {
    ast_kind_t  kind;
    union {
        struct {
            struct ast_node *   left;
            struct ast_node *   right;
        } bin;

        struct ast_node     *   child;
        const char          *   val;
    } un;
};

static struct ast_node *
binop_new(ast_kind_t kind, struct ast_node *left, struct ast_node *right)
{
    struct ast_node *ret;
    ret = calloc(1, sizeof(*ret));

    ret->un.bin.left = left;
    ret->un.bin.right = right;
    ret->kind = kind;

    return ret;
}

static struct ast_node *
terminal_new(const char *val)
{
    struct ast_node *ret;

    ret = calloc(1, sizeof(*ret));

    ret->kind = AST_TERMINAL;
    ret->un.val = val;

    return ret;
}

static void
destroy_ast_node(struct ast_node *node)
{
    switch (node->kind) {
        case AST_BINOP_ADD:
        case AST_BINOP_AND:
        case AST_BINOP_DIV:
        case AST_BINOP_EQ:
        case AST_BINOP_GE:
        case AST_BINOP_GT:
        case AST_BINOP_LE:
        case AST_BINOP_LT:
        case AST_BINOP_MUL:
        case AST_BINOP_OR:
        case AST_BINOP_SUB:
            destroy_ast_node(node->un.bin.left);
            destroy_ast_node(node->un.bin.right);
            break;
        default:
            break;
    }

    free(node);
}

static const char *
peek_token(struct token_stream *tokens)
{
    if (tokens->pos >= tokens->len) return NULL;

    return tokens->tokens[tokens->pos];
}

static const char *
read_token(struct token_stream *tokens)
{
    if (tokens->pos >= tokens->len) return NULL;

    return tokens->tokens[tokens->pos++];  
}

static bool
match_token(struct token_stream *tokens, const char *val)
{
    const char *head;

    head = peek_token(tokens);

    if (!head) return NULL;

    return strcmp(head, val) == 0;
}

static struct ast_node * parse_expr (struct token_stream *);

static struct ast_node *
parse_terminal(struct token_stream *tokens)
{
    const char *val;
    struct ast_node *ret;

    if (match_token(tokens, "(")) {
        read_token(tokens);

        ret = parse_expr(tokens);

        if (!ret) {
            return NULL;
        }

        if (!match_token(tokens, ")")) {
            fprintf(stderr, "syntax error: expected ')'\n");
            return NULL;
        }

        read_token(tokens);

        return ret;
    } else {
        val = read_token(tokens);

        if (val) return terminal_new(val);
    }

    return NULL;
}

static struct ast_node *
parse_div_mul(struct token_stream *tokens)
{
    struct ast_node *left;

    left = parse_terminal(tokens);

    while (match_token(tokens, "*") || match_token(tokens, "/")) {
        const char *op;
        struct ast_node *right;

        op = read_token(tokens);

        right = parse_terminal(tokens);

        if (!right) {
            destroy_ast_node(left);
            return NULL;
        }

        switch (op[0]) {
            case '*':
                left = binop_new(AST_BINOP_MUL, left, right);
                break;
            case '/':
                left = binop_new(AST_BINOP_DIV, left, right);
                break;
        }
    }

    return left;
}

static struct ast_node *
parse_add_sub(struct token_stream *tokens)
{
    struct ast_node *left;

    left = parse_div_mul(tokens);

    while (match_token(tokens, "+") || match_token(tokens, "-")) {
        const char *op;
        struct ast_node *right;

        op = read_token(tokens);

        right = parse_div_mul(tokens);

        if (!right) {
            destroy_ast_node(left);
            return NULL;
        }

        switch (op[0]) {
            case '+':
                left = binop_new(AST_BINOP_ADD, left, right);
                break;
            case '-':
                left = binop_new(AST_BINOP_SUB, left, right);
                break;
        }
    }

    return left;
}

static struct ast_node *
parse_relational(struct token_stream *tokens)
{
    struct ast_node *left;

    left = parse_add_sub(tokens);

    while ( match_token(tokens, ">") || match_token(tokens, ">=") ||
            match_token(tokens, "<") || match_token(tokens, "<="))
    {
        const char *op;
        struct ast_node *right;

        op = read_token(tokens);

        right = parse_add_sub(tokens);

        if (!right) {
            destroy_ast_node(left);
            return NULL;
        }

        if (strcmp(op, "<=") == 0) left = binop_new(AST_BINOP_LE, left, right);
        else if (strcmp(op, ">=") == 0) left = binop_new(AST_BINOP_GE, left, right);
        else {
            switch (op[0]) {
                case '<':
                    left = binop_new(AST_BINOP_LT, left, right);
                    break;
                case '>':
                    left = binop_new(AST_BINOP_GT, left, right);
                    break;
            }
        }
    }

    return left;
}

static struct ast_node *
parse_equality(struct token_stream *tokens)
{
    struct ast_node *left;

    left = parse_relational(tokens);

    while ( match_token(tokens, "=") || match_token(tokens, "==")) {
        struct ast_node *right;

        read_token(tokens);

        right = parse_relational(tokens);

        if (!right) {
            destroy_ast_node(left);
            return NULL;
        }

        left = binop_new(AST_BINOP_EQ, left, right);
    }

    return left;
}

static struct ast_node *
parse_and(struct token_stream *tokens)
{
    struct ast_node *left;

    left = parse_equality(tokens);

    while (match_token(tokens, "&")) {
        struct ast_node *right;

        read_token(tokens);

        right = parse_equality(tokens);

        if (!right) {
            destroy_ast_node(left);
            return NULL;
        }

        left = binop_new(AST_BINOP_AND, left, right);
    }

    return left;
}

static struct ast_node *
parse_or(struct token_stream *tokens)
{
    struct ast_node *left;

    left = parse_and(tokens);

    while (match_token(tokens, "|")) {
        struct ast_node *right;

        read_token(tokens);

        right = parse_and(tokens);

        if (!right) {
            destroy_ast_node(left);
            return NULL;
        }

        left = binop_new(AST_BINOP_OR, left, right);
    }

    return left;
}

static struct ast_node *
parse_expr(struct token_stream *tokens)
{
    return parse_or(tokens);
}

static int
eval_terminal(struct ast_node *val)
{
    return atoi(val->un.val);
}

static int eval_ast_node(struct ast_node *);

static int
eval_binop(struct ast_node *val)
{
    int left;
    int right;

    left = eval_ast_node(val->un.bin.left);
    right = eval_ast_node(val->un.bin.right);

    switch (val->kind) {
        case AST_BINOP_ADD:
            return left + right;
        case AST_BINOP_AND:
            return left != 0 && right != 0 ? left : right;
        case AST_BINOP_DIV:
            return left / right;
        case AST_BINOP_EQ:
            return left != right ? 1 : 0;
        case AST_BINOP_GE:
            return left >= right ? 0 : 1;
        case AST_BINOP_GT:
            return left > right ? 0 : 1;
        case AST_BINOP_LE:
            return left <= right ? 0 : 1;
        case AST_BINOP_LT:
            return left < right ? 0 : 1;
        case AST_BINOP_MUL:
            return left * right;
        case AST_BINOP_OR:
            return left != 0 || right != 0 ? left : right;
        case AST_BINOP_SUB:
            return left - right;
        default:
            return 0;
    }
}

static int
eval_ast_node(struct ast_node *node)
{
    switch (node->kind) {
        case AST_BINOP_ADD:
        case AST_BINOP_DIV:
        case AST_BINOP_EQ:
        case AST_BINOP_GE:
        case AST_BINOP_GT:
        case AST_BINOP_LE:
        case AST_BINOP_LT:
        case AST_BINOP_MUL:
        case AST_BINOP_SUB:
        case AST_BINOP_AND:
        case AST_BINOP_OR:
            return eval_binop(node);
        case AST_TERMINAL:
            return eval_terminal(node);
        default:
            return -1;
    }
}

int
main(int argc, const char *argv[])
{
    int rc;
    int val;
    struct token_stream token_stream;
    struct ast_node * root;

    token_stream.pos = 0;
    token_stream.len = argc - 1;
    token_stream.tokens = argv + 1;

    root = parse_expr(&token_stream);

    if (!root) return -1;

    val = eval_ast_node(root);

    printf("%d\n", val);

    switch (root->kind) {
        case AST_BINOP_EQ:
        case AST_BINOP_GE:
        case AST_BINOP_GT:
        case AST_BINOP_LE:
        case AST_BINOP_LT:
        case AST_BINOP_AND:
        case AST_BINOP_OR:
            rc = val;
            break;
        default:
            rc = 0;
            break;
    }

    return rc;
}