#include <stdio.h>
#include <sys/stat.h>

int
main (int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0777) != 0) {
            perror("mkdir");
            return -1;
        }
    }
    
    return 0;
}

