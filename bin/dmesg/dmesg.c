#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
    FILE *fp;

    fp = fopen("/dev/kmsg", "r");

    if (fp) {
        size_t nread;
        char buffer[1024];

        while ((nread = fread(buffer, 1, 1024, fp)) > 0) {
            fwrite(buffer, 1, nread, stdout);
        }
    
        puts(""); 
        return 0;
    } else {
        perror("dmesg");
    }

    return -1;
}

