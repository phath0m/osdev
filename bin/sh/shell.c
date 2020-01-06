#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>

#include "dict.h"
#include "list.h"
#include "parser.h"
#include "tokenizer.h"

typedef int (*command_handler_t)(const char *name, int argc, const char *argv[]);

static void eval_ast_node(struct ast_node *node);

struct dict builtin_commands;

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    size_t pos;
    int c;

    if (lineptr == NULL || stream == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }

    if (*lineptr == NULL) {
        *lineptr = malloc(128);
        if (*lineptr == NULL) {
            return -1;
        }
        *n = 128;
    }

    pos = 0;
    while(c != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n + (*n >> 2);
            if (new_size < 128) {
                new_size = 128;
            }
            char *new_ptr = realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }
            *n = new_size;
            *lineptr = new_ptr;
        }

        ((unsigned char *)(*lineptr))[pos ++] = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    (*lineptr)[pos] = '\0';
    return pos;
}

static bool
is_empty_or_comment(const char *str)
{
    if (*str == '#') {
        return true;
    }

    bool empty = true;

    while (*str) {
        if (!isspace((int)*str)) {
            empty = false;
        }
        str++;
    }

    return empty;
}

static bool
search_for_binary(const char *name, char *resolved)
{
    if (!strncmp(name, "/", 1) || !strncmp(name, "./", 2)) {
        strcpy(resolved, name);
        return true;
    }

    const char *path[] = {"/bin", NULL};

    for (int i = 0; path[i]; i++) {
        bool succ = false;
        DIR *dirp = opendir(path[i]);

        struct dirent *dirent;

        while ((dirent = readdir(dirp))) { 
            if (strcmp(dirent->d_name, name) == 0) {
                sprintf(resolved, "%s/%s", path[i], name);
                succ = true;
                break;
            }
        }

        closedir(dirp);

        if (succ) {
            return true;
        }
    }

    return false;
}

static int
exec_binary(const char *name, int argc, const char *argv[])
{
    char full_path[512];

    if (!search_for_binary(name, full_path)) {
        printf("sh: could not find %s\n", name);
        return -1;
    }

    int id = fork();
    int status = -1;

    if (id == 0) {
        execv(full_path, (char ** const)argv);
    } else {
        wait(&status);
    }

    return status;
}

static int
exec_command(struct ast_node *root)
{
    char *name = (char*)root->value;
    char *argv[512];

    argv[0] = (char*)name;

    list_iter_t iter;
    list_get_iter(&root->children, &iter);

    struct ast_node *arg;

    int i;

    for (i = 1; iter_move_next(&iter, (void**)&arg); i++) {
        argv[i] = (char*)arg->value;
    }

    argv[i] = 0;

    command_handler_t func;

    if (dict_get(&builtin_commands, name, (void**)&func)) {
        return func(argv[0], i, (const char**)argv);
    }

    return exec_binary(argv[0], i, (const char**)argv);
}

static int
handle_pipe(struct ast_node *root)
{
    list_iter_t iter;

    list_get_iter(&root->children, &iter);

    struct ast_node *left;
    struct ast_node *right;
    
    iter_move_next(&iter, (void**)&left);
    iter_move_next(&iter, (void**)&right);

    int fds[2];

    pipe(fds);

    int status;
    pid_t child = fork();

    if (child != 0) {
        int tmp = dup(STDIN_FILENO);
        dup2(fds[0], STDIN_FILENO);
        close(fds[1]);
        eval_ast_node(right);
        close(STDIN_FILENO);
        wait(&status);
        dup2(tmp, STDIN_FILENO);

    } else {
        
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        eval_ast_node(left);
        close(STDOUT_FILENO);
        exit(0);
    }
    return 0;
}

static int
handle_file_write(struct ast_node *root)
{
    list_iter_t iter;

    list_get_iter(&root->children, &iter);

    struct ast_node *left;
    struct ast_node *right;

    iter_move_next(&iter, (void**)&left);
    iter_move_next(&iter, (void**)&right);

    int file_fd = open((const char*)right->value, O_WRONLY | O_CREAT);

    if (file_fd < 0) {
        return -1;
    }

    int tmp = dup(STDOUT_FILENO);

    dup2(file_fd, STDOUT_FILENO);

    eval_ast_node(left);

    close(file_fd);

    dup2(tmp, STDOUT_FILENO);

    return 0;
}

static void
eval_ast_node(struct ast_node *node)
{
    switch (node->node_class) {
        case AST_COMMAND:
            exec_command(node);
            break;
        case AST_FILE_WRITE:
            handle_file_write(node);
            break;
        case AST_PIPE:
            handle_pipe(node);
            break;
        default:
            break;
    }
}

static void
eval_command(const char *cmd)
{
    if (is_empty_or_comment(cmd)) {
        return;
    }

    struct list tokens;
    struct tokenizer scanner;
    memset(&tokens, 0, sizeof(struct list));

    tokenizer_init(&scanner, cmd);
    tokenizer_scan(&scanner, &tokens);
    
    struct parser parser;

    parser_init(&parser, &tokens);

    struct ast_node *root = parser_parse(&parser);
    
    eval_ast_node(root);

    ast_node_destroy(root);
    
    list_destroy(&tokens, true);
}

void
run_file(const char *path)
{
    FILE *fp = fopen(path, "r");

    size_t len = 0;
    ssize_t read;

    char *line = NULL;

    while ((read = getline(&line, &len, fp)) != -1) {
        eval_command(line);
    }

    fclose(fp);
}

int
builtin_cd(const char *name, int argc, const char *argv[])
{
    if (argc == 2) {
        if (chdir(argv[1])) {
            printf("cd: %s: No such directory\n", argv[1]);
            return -1;
        }
    } else {
        chdir("/");
    }

    return 0;
}

int
builtin_setenv(const char *name, int argc, const char *argv[])
{
    if (argc == 1) {
        extern char **environ;

        for (int i = 0; environ[i]; i++) {
            printf("%s\n", environ[i]);
        }
        return 0;
    }

    if (argc != 3) {
        return -1;
    }

    setenv(argv[1], argv[2], true);

    return 0;
}

int
builtin_source(const char *name, int argc, const char *argv[])
{
    if (argc != 2) {
        return -1;
    }

    run_file(argv[1]);

    return 0;
}

void
shell_repl()
{
    for (;;) {
        char buffer[512];
        memset(buffer, 0, 512);

        fputs("# ", stdout);

        fflush(stdout);

        while (fgets(buffer, 512, stdin) == NULL);

        if (strlen(buffer) > 0) {
            eval_command(buffer);
        }
    }

}

int
main(int argc, const char *argv[])
{
    memset(&builtin_commands, 0, sizeof(struct dict));

    dict_set(&builtin_commands, "cd", builtin_cd);
    dict_set(&builtin_commands, "setenv", builtin_setenv);
    dict_set(&builtin_commands, "source", builtin_source);

    if (argc > 1) {
        run_file(argv[1]);
    } else {
        shell_repl();
    }
    return 0;
}
