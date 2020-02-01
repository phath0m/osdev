#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, const char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: newtty <tty>\n");
        return -1;
    }
    
    const char *console = argv[1];

    int tty = open(console, O_RDWR);

    if (!tty) {
        perror("newtty");
        return -1;
    }

    for (int i = 0; i < 3; i++) {
        close(i);    
        dup2(tty, i);
    }

    static char *sh_argv[2] = {
        "/usr/bin/login",
        NULL
    };

    execv(sh_argv[0], sh_argv);

    return 0;
}

