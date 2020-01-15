#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, const char *argv[])
{
    const char *console = argv[1];

    int tty = open(console, O_RDWR);

    for (int i = 0; i < 3; i++) {
        close(i);    
        dup2(tty, i);
    }

    static char *sh_argv[2] = {
        "-sh",
        NULL
    };

    execv("/bin/sh", sh_argv);

    return 0;
}
