#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void exit(int code);
extern int main(int argc, const char *argv[], int envc, const char *envp[]);

static void
default_exit_handler(int sig)
{
    switch(sig) {
        case SIGSEGV:
            fprintf(stderr, "Segmentation Fault\n");
            break;
        case SIGILL:
            fprintf(stderr, "Illegal Instruction\n");
            break;
    }
    exit(-1);
}

void
_premain(int argc, const char **argv, int envc, const char **envp)
{
    int i;

    extern char **environ;
    
    environ = (char**)calloc(1, sizeof(char*) * 128);

    for (i = 0; i < envc; i++) {
        environ[i] = envp[i];
    }

    signal(SIGILL, default_exit_handler);
    signal(SIGINT, default_exit_handler);
    signal(SIGSEGV, default_exit_handler);
    signal(SIGTERM, default_exit_handler);

    int ex = main(argc, argv, envc, envp);

    exit(ex);
}
