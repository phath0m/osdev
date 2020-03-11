#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: sleep SECONDS\n");
        return -1;
    }
    
    int timeout = strtol(argv[1], NULL, 10);

    sleep(timeout);
    
    return 0;
}

