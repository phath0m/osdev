#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: chmod MODE FILE\n");
        return -1; 
    }

    char *mode_str = argv[1];
    char *file = argv[2];

    mode_t mode = strtol(mode_str, NULL, 8);

    if (chmod(file, mode) != 0) {
        perror("chmod");

        return -1;
    }

    return 0;
}

