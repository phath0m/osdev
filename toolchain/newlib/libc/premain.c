#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void exit(int code);
extern int main(int argc, const char *argv[], int envc, const char *envp[]);

void
_premain(int argc, const char **argv, int envc, const char **envp)
{
    int i;

    extern char **environ;
    
    environ = (char**)calloc(1, sizeof(char*) * 128);

    for (i = 0; i < envc; i++) {
        environ[i] = envp[i];
    }

    int ex = main(argc, argv, envc, envp);

    exit(ex);
}
