#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        fputs("usage: chroot NEW_ROOT CMD ...\n", stderr);
        return -1;
    }
   
    char *newroot = argv[1];
    char *cmd = argv[2];

    if (chroot(newroot) != 0) {
        perror("chroot");
        return -1;
    }

    if (chdir("/") != 0) {
        perror("chroot");
        return -1;
    }

    execv(cmd, &argv[2]);

    perror("chroot");

    return -1;
}

