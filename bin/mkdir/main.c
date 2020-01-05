#include <stdio.h>
#include <sys/stat.h>

int
main (int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0700) != 0) {
            perror("mkdir");
        }
    }
    
    return 0;
}

