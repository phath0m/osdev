#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <fcntl.h>

#include <collections/dict.h>
#include <collections/list.h>
#include "parser.h"
#include "tokenizer.h"

typedef int (*command_handler_t)(const char *, int, const char *[]);

static void eval_ast_node(struct ast_node *);

struct dict builtin_commands;

ssize_t getline(char **, size_t *, FILE *);

static bool
is_empty_or_comment(const char *str)
{
    bool empty;

    if (*str == '#') {
        return true;
    }

    empty = true;

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
    static char path_copy[1024];

    bool succ;
    char *path;
    char *searchdir;
    DIR *dirp;

    if (!strncmp(name, "/", 1) || !strncmp(name, "./", 2)) {
        strcpy(resolved, name);
        return true;
    }

    path = getenv("PATH");

    if (!path) {
        return false;
    }

    strncpy(path_copy, path, 1024);

    searchdir = strtok(path_copy, ":");
    
    while (searchdir != NULL) {
        struct dirent *dirent;
        succ = false;

        dirp = opendir(searchdir);

        if (!dirp) {
            searchdir = strtok(NULL, ":");
            continue;
        }

        while ((dirent = readdir(dirp))) { 
            if (strcmp(dirent->d_name, name) == 0) {
                sprintf(resolved, "%s/%s", searchdir, name);
                succ = true;
                break;
            }
        }

        closedir(dirp);

        if (succ) {
            return true;
        }

        searchdir = strtok(NULL, ":");
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

    return execv(full_path, (char ** const)argv);
}

static int
spawn_binary(const char *name, int argc, const char *argv[])
{
    int id;
    int status;
    char full_path[512];

    if (!search_for_binary(name, full_path)) {
        printf("sh: could not find %s\n", name);
        return -1;
    }

    id = fork();
    status = -1;

    if (id == 0) {
        setpgid(0, 0);
        tcsetpgrp(STDIN_FILENO, getpid());
        execv(full_path, (char ** const)argv);
    } else {
        wait(&status);
    }

    return status;
}

static int
run_command(struct ast_node *root)
{
    command_handler_t func;

    int i;
    list_iter_t iter;

    char *name;
    char *argv[512];

    struct ast_node *arg;

    name = root->value;
    argv[0] = root->value;

    list_get_iter(&root->children, &iter);

    for (i = 1; iter_move_next(&iter, (void**)&arg); i++) {
        argv[i] = arg->value;
    }

    argv[i] = 0;

    if (dict_get(&builtin_commands, name, (void**)&func)) {
        return func(argv[0], i, (const char**)argv);
    }

    return spawn_binary(argv[0], i, (const char**)argv);
}

static int
handle_assign(struct ast_node *root)
{
    list_iter_t iter;
    struct ast_node *left;

    list_get_iter(&root->children, &iter);

    iter_move_next(&iter, (void**)&left);

    // temporary until I implement export
    setenv((char*)root->value, (char*)left->value, 1);

    return 0;
}

static int
handle_pipe(struct ast_node *root)
{
    int status;
    list_iter_t iter;
    pid_t child;

    int fds[2];

    struct ast_node *left;
    struct ast_node *right;
    
    list_get_iter(&root->children, &iter);
    
    iter_move_next(&iter, (void**)&left);
    iter_move_next(&iter, (void**)&right);

    pipe(fds);

    child = fork();

    if (child != 0) {
        int tmp;
        tmp = dup(STDIN_FILENO);
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
    int file_fd;
    int tmp;
    list_iter_t iter;

    struct ast_node *left;
    struct ast_node *right;


    list_get_iter(&root->children, &iter);

    iter_move_next(&iter, (void**)&left);
    iter_move_next(&iter, (void**)&right);

    file_fd = open((const char*)right->value, O_WRONLY | O_CREAT);

    if (file_fd < 0) {
        return -1;
    }

    tmp = dup(STDOUT_FILENO);

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
        case AST_ASSIGNMENT:
            handle_assign(node);
            break;
        case AST_COMMAND:
            run_command(node);
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
    struct list tokens;
    struct tokenizer scanner;
    struct parser parser;

    struct ast_node *root;

    if (is_empty_or_comment(cmd)) {
        return;
    }

    memset(&tokens, 0, sizeof(struct list));

    tokenizer_init(&scanner, cmd);
    tokenizer_scan(&scanner, &tokens);
    
    parser_init(&parser, &tokens);

    root = parser_parse(&parser);
    
    eval_ast_node(root);

    ast_node_destroy(root);
    
    list_destroy(&tokens, true);
}

void
run_file(const char *path)
{
    size_t len;
    ssize_t nread;

    char *line;
    FILE *fp;

    fp = fopen(path, "r");
    len = 0;

    while ((nread = getline(&line, &len, fp)) != -1) {
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
builtin_clear(const char *name, int argc, const char *argv[])
{
    fputs("\x1B" "c", stdout);
    return 0;
}

int
builtin_exec(const char *name, int argc, const char *argv[])
{
    const char *prog;

	if (argc < 2) {
		return 0;
	}

    prog = argv[1];
    
    return exec_binary(prog, argc - 1, &argv[1]);
}

int
builtin_exit(const char *name, int argc, const char *argv[])
{
    int exit_code;
    
    exit_code = 0;

    if (argc > 1) {
        exit_code = atoi(argv[1]);
    }

    exit(exit_code);
}

int
builtin_pwd(const char *name, int argc, const char *argv[])
{
    char cwd[512];

    getcwd(cwd, 512);

    puts(cwd);

    return 0;
}

int
builtin_setenv(const char *name, int argc, const char *argv[])
{
    extern char **environ;

    int i;

    if (argc == 1) {
        for (i = 0; environ[i]; i++) {
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

static void
print_prompt_escape(char ch)
{
    char cwd[512];

    switch (ch) {
        case 'e': {
            putchar('\x1B');
            break;              
        }              
        case 'u': {
            fputs(getlogin(), stdout);
            break;
        }              
        case 'w': {
            getcwd(cwd, 512);
            fputs(cwd, stdout);
            break;
        }
        case '$': {
            if (getuid() == 0) putchar('#');
            else putchar('%');          
            break;
        }
        case '\\': {
            break;
        }

    }
}

void
print_prompt(const char *prompt)
{
    bool escaped;
    char ch;
    int i;

    escaped = false;

    for (i = 0; i < strlen(prompt); i++) {
        ch = prompt[i];

        if (ch == '\\' && !escaped) {
            escaped = true;
            continue;
        }
        
        if (!escaped) {
            putchar(ch);
        } else {
            print_prompt_escape(ch);
            escaped = false;
        }
    }
}


static void
load_rc()
{
    char *home;

    char rc_path[512];

    if (access("/etc/shrc", R_OK) == 0) {
        run_file("/etc/shrc");
    }

    home = getenv("HOME");

    if (!home) {
        return;
    }

    sprintf(rc_path, "%s/.shrc", home);

    if (access(rc_path, R_OK) == 0) {
        run_file(rc_path);
    }
}

void
shell_repl()
{
    ioctl(STDIN_FILENO, TIOCSCTTY, NULL);

    setenv("PS1", "\\$ ", false);
    load_rc();

    for (;;) {
        char buffer[512];
        memset(buffer, 0, 512);

        print_prompt(getenv("PS1"));
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
    dict_set(&builtin_commands, "clear", builtin_clear);
    dict_set(&builtin_commands, "exec", builtin_exec);
    dict_set(&builtin_commands, "exit", builtin_exit);
    dict_set(&builtin_commands, "pwd", builtin_pwd);
    dict_set(&builtin_commands, "setenv", builtin_setenv);
    dict_set(&builtin_commands, "source", builtin_source);

    setsid();

    if (argc > 1) {
        run_file(argv[1]);
    } else {
        shell_repl();
    }
    return 0;
}
