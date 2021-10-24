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
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>

#include <collections/dict.h>
#include <collections/list.h>
#include "parser.h"
#include "tokenizer.h"

typedef int (*command_handler_t)(const char *, int, const char *[]);

static int eval_command(const char *);
static int eval_ast_node(struct ast_node *);
static char *substitute_string_variables(char *);

struct dict builtin_commands;
struct dict variables;

static size_t
new_string_size(size_t strsize)
{
    return (strsize + 1024) & ~0x3FF;
}

static char *
strdyncat(char **ptr, size_t *bufsize, char *substr, size_t substrlen)
{
    size_t oldstrlen;
    size_t newstrlen;

    if (substrlen == 0) {
        return *ptr;
    }

    if (!*ptr) {
        *bufsize = new_string_size(substrlen);
        *ptr = calloc(1, *bufsize);
        strncat(*ptr, substr, substrlen);    
        return *ptr;
    }

    oldstrlen = strlen(*ptr);
    newstrlen = oldstrlen + substrlen;

    if (newstrlen >= *bufsize) {
        *bufsize = new_string_size(newstrlen);
        *ptr = realloc(*ptr, *bufsize);
    }

    strncat(*ptr, substr, substrlen);
    return *ptr;
}

static char *
resolve_variable(char *name, size_t *sizep)
{
    char *val;

    if (!dict_get(&variables, name, (void**)&val))
        val = getenv(name);

    if (val) {
        *sizep = strlen(val);
        return val;
    }

    *sizep = 0;

    return "";
}

static void
substitute_command(char *orig, int *p, char **ptr, size_t *ptr_len)
{
    char ch;
    int i;
    pid_t child;

    char *cmd_subst;
    char *output;
    size_t output_len;

    char cmd[512];
    int fds[2];

    i = 0;
    output = NULL;

    while ((ch = *(orig++)) && ch != '`') {
        cmd[i] = ch;
        i++;
    }

    cmd[i] = 0;
    *p += i + 1;

    cmd_subst = substitute_string_variables(cmd);

    pipe(fds);

    child = fork();

    if (child != 0) {
        int status;
        int tmp;
        ssize_t nread;
        char buf[512];

        tmp = fds[0];

        close(fds[1]);

        while ((nread = read(tmp, buf, sizeof(buf))) > 0) {
            strdyncat(&output, &output_len, buf, nread);
        }

        close(tmp);
        wait(&status);
    } else {
        close(STDOUT_FILENO);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        eval_command(cmd_subst);
        close(STDOUT_FILENO);
        exit(0);
    }

    /* trim out trailing line break */

    for (i = strlen(output) - 1; i >= 0 && output[i] == '\n'; i--) {
        output[i] = 0;
    }

    if (output) strdyncat(ptr, ptr_len, output, strlen(output));
    free(output);
    free(cmd_subst);
}


static void
substitute_variable_name(char *orig, int *p, char **ptr, size_t *ptr_len)
{
    char ch;
    int i;
    size_t subst_len;
    char *subst_val;
    char var_name[256];

    i = 0;

    if (*orig == '$') {
        subst_val = "$";
        subst_len = 1;
        i = 1;
    } else {
        while ((ch = *(orig++))) {
            if (!isalnum((int)ch)) break;
            var_name[i++] = ch;
        }

        var_name[i] = '\x00';

        subst_val = resolve_variable(var_name, &subst_len);
    }

    strdyncat(ptr, ptr_len, subst_val, subst_len);

    *p += i;
}

static void
substitute_string_literal(char *orig, int *p, char **ptr, size_t *ptr_len)
{
    char ch;
    int lit_len;
    char *lit;

    lit = orig ;
    lit_len = 0;

    while ((ch = *(orig++)) && ch != '\'') {
        lit_len++;
    }

    strdyncat(ptr, ptr_len, lit, lit_len);

    *p += lit_len;
}

static char *
substitute_string_variables(char *orig)
{
    int i;
    size_t ptr_len;
    char *ptr;

    ptr = NULL;
    i = 0;

    while (orig[i]) {
        switch (orig[i]) {
            case '$':
                i += 1;
                substitute_variable_name(orig + i, &i, &ptr, &ptr_len);
                break;
            case '\'':
                i += 1;
                substitute_string_literal(orig + i, &i, &ptr, &ptr_len);
                break;
            case '`':
                i += 1;
                substitute_command(orig + i, &i, &ptr, &ptr_len);
                break;
            default:
                strdyncat(&ptr, &ptr_len, orig + i, 1);
                i++;
                break;
        }
    }

    return ptr;
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
    int rc;
    list_iter_t iter;

    char *name;
    char *argv[512];

    struct ast_node *arg;

    name = root->value;
    argv[0] = root->value;

    list_get_iter(&root->children, &iter);

    for (i = 1; iter_next_elem(&iter, (void**)&arg); i++) {
        argv[i] = substitute_string_variables(arg->value);
    }

    argv[i] = 0;

    if (dict_get(&builtin_commands, name, (void**)&func)) {
        rc = func(argv[0], i, (const char**)argv);
    } else {
        rc = spawn_binary(argv[0], i, (const char**)argv);
    }

    for (i = 1; argv[i]; i++) {
        free(argv[i]);
    }

    return rc;
}

static int
handle_assign(struct ast_node *root)
{
    list_iter_t iter;
    char *val;
    struct ast_node *left;

    list_get_iter(&root->children, &iter);
    iter_next_elem(&iter, (void**)&left);

    if (dict_get(&variables, root->value, (void**)&val)) {
        free(val);
    }

    val = substitute_string_variables(left->value);

    dict_set(&variables, root->value, val);

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
    
    iter_next_elem(&iter, (void**)&left);
    iter_next_elem(&iter, (void**)&right);
 
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

    iter_next_elem(&iter, (void**)&left);
    iter_next_elem(&iter, (void**)&right);

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

static int
handle_command_list(struct ast_node *root)
{
    int rc;
    list_iter_t iter;

    struct ast_node *child;

    rc = 0;
    list_get_iter(&root->children, &iter);

    while (iter_next_elem(&iter, (void**)&child)) {
        rc = eval_ast_node(child);
    }
    
    return rc;
}

static int
handle_logical_and(struct ast_node *root)
{
    int rc;
    list_iter_t iter;
    struct ast_node *left;
    struct ast_node *right;
    
    list_get_iter(&root->children, &iter);
    
    iter_next_elem(&iter, (void**)&left);
    iter_next_elem(&iter, (void**)&right);

    rc = eval_ast_node(left);

    if (rc == 0) {
        rc = eval_ast_node(right);
    }

    return rc;

}

static int
handle_logical_or(struct ast_node *root)
{
    int rc;
    list_iter_t iter;
    struct ast_node *left;
    struct ast_node *right;
    
    list_get_iter(&root->children, &iter);
    
    iter_next_elem(&iter, (void**)&left);
    iter_next_elem(&iter, (void**)&right);

    rc = eval_ast_node(left);

    if (rc != 0) {
        rc = eval_ast_node(right);
    }

    return rc;
}

static int
handle_if_stmt(struct ast_node *root)
{
    int rc;
    list_iter_t iter;
    struct ast_node *expr;
    struct ast_node *body;
    struct ast_node *else_body;

    list_get_iter(&root->children, &iter);
    
    iter_next_elem(&iter, (void**)&expr);
    iter_next_elem(&iter, (void**)&body);
    iter_next_elem(&iter, (void**)&else_body);

    rc = eval_ast_node(expr);

    if (rc == 0) {
        rc = eval_ast_node(body);
    } else {
        rc = eval_ast_node(else_body); 
    }

    return rc;
}

static int
handle_while_stmt(struct ast_node *root)
{
    int rc;
    list_iter_t iter;
    struct ast_node *expr;
    struct ast_node *body;

    list_get_iter(&root->children, &iter);
    
    iter_next_elem(&iter, (void**)&expr);
    iter_next_elem(&iter, (void**)&body);

    while ((rc = eval_ast_node(expr)) == 0) {
        eval_ast_node(body);
    }

    return rc;
}

static int
eval_ast_node(struct ast_node *node)
{
    switch (node->node_class) {
        case AST_ASSIGNMENT:
            return handle_assign(node);
        case AST_COMMAND:
            return run_command(node);
        case AST_COMMAND_LIST:
            return handle_command_list(node);
        case AST_FILE_WRITE:
            return handle_file_write(node);
        case AST_PIPE:
            return handle_pipe(node);
        case AST_LOGICAL_AND:
            return handle_logical_and(node);
        case AST_LOGICAL_OR:
            return handle_logical_or(node);
        case AST_IF_STMT:
            return handle_if_stmt(node);
        case AST_WHILE_STMT:
            return handle_while_stmt(node);
        default:
            return -1;
    }
}

static int
eval_command(const char *cmd)
{
    int rc;
    struct list tokens;
    struct tokenizer scanner;
    struct parser parser;
    struct ast_node *root;

    LIST_INIT(tokens);

    tokenizer_init(&scanner, cmd);
    tokenizer_scan(&scanner, &tokens);
    
    parser_init(&parser, &tokens);

    root = parser_parse(&parser);

    if (!root) {
        rc = -1;
        goto cleanup;
    }

    rc = eval_ast_node(root);
    ast_node_destroy(root);

cleanup:
    list_fini(&tokens, LIST_DESTROY_DEFAULT_CB, NULL);

    return rc;
}

int
run_file(const char *path)
{
    char *contents;
    FILE *fp;

    struct stat sb;

    stat(path, &sb);

    contents = calloc(sb.st_size+1, 1);

    fp = fopen(path, "r");

    if (!fp) {
        perror("sh");
        return -1;
    }


    fread(contents, 1, sb.st_size, fp);
    eval_command(contents);
    fclose(fp);

    return 0;
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
builtin_export(const char *name, int argc, const char *argv[])
{
    char *val;

    if (argc == 2 && dict_get(&variables, argv[1], (void**)&val)) {
        setenv(argv[1], val, 1);
    }

    return 0;
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
    memset(&builtin_commands, 0, sizeof(builtin_commands));
    memset(&variables, 0, sizeof(variables));

    dict_set(&builtin_commands, "cd", builtin_cd);
    dict_set(&builtin_commands, "clear", builtin_clear);
    dict_set(&builtin_commands, "exec", builtin_exec);
    dict_set(&builtin_commands, "exit", builtin_exit);
    dict_set(&builtin_commands, "export", builtin_export);
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
