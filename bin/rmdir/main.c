#include <stdio.h>
#include <unistd.h>

int
main (int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (rmdir(argv[i]) != 0) {
            perror("rmdir");
            return -1;
        }
    }
    
    return 0;
}

