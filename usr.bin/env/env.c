#include <stdio.h>

int
main (int argc, char *argv[])
{
    extern char **environ;

    char **envp = environ;

    for (; *envp; envp++) {
        puts(*envp);
    }

    return 0;
}

