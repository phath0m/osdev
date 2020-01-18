#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
    if (argc > 1) {
        FILE *fp = fopen(argv[1], "r");

        if (fp) {
            size_t nread;
            char buffer[1024];

            while ((nread = fread(buffer, 1, 1024, fp)) > 0) {
                fwrite(buffer, 1, nread, stdout);
            }
        
            puts(""); 
            return 0;
        } else {
            perror("cat");
        }
    }
    return -1;
}

