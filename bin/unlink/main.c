#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (unlink(argv[i]) != 0) {
            perror("unlink");
            return -1;
        }
    }
    
    return 0;
}

