#include <stdio.h>

int
main (int argc, char *argv[])
{
    if (argc > 1) {
        FILE *fp = fopen(argv[1], "r");

        if (fp) {
            char buffer[1024];
        
            while (fread (buffer, 1, 1024, fp) > 0) {
                printf ("%s", buffer);
            }
        
            printf ("\n");
            
            return 0;
        }
    }
    return -1;
}

