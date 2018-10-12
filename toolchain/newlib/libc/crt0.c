#include <fcntl.h>
#include <stdio.h>
#include <string.h>

extern void exit(int code);
extern int main(int argc, const char *argv[], int envc, const char *envp[]);

void
_premain(int argc, const char **argv, int envc, const char **envp)
{
    int ex = main(argc, argv, envc, envp);
    exit(ex);
}
