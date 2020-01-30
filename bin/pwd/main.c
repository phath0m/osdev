#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    char path[512];

    getcwd(path, 512);

    puts(path);

    return 0;
}

