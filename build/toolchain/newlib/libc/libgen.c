#include <string.h>


char *
dirname(char *path)
{
    static char buf[512];

    strncpy(buf, path, sizeof(buf));

    *strrchr(buf, '/') = 0;

    return buf;
}

char *
basename(char *path)
{
    return strrchr(path, '/') + 1;
}

