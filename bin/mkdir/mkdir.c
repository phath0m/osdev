#include <stdio.h>
#include <sys/stat.h>

int
main (int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0777) != 0) {
            perror("mkdir");
            return -1;
        }
    }
    
    return 0;
}

