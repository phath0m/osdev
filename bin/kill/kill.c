#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>


static void
usage()
{
    fprintf(stderr, "usage: kill [<sigpsec>] <pid>\n");
}

int
main(int argc, const char *argv[])
{
    if (argc < 2) {
        usage();
        return -1;
    }

    int sig = SIGTERM;

    if (!strncmp(argv[1], "-", 1)) {
        
        if (argc != 3) {
            usage();
            return -1;
        }

        const char *opt = &argv[1][1];

        sig = atoi(opt);
    }

    int pid = atoi(argv[argc - 1]);

    if (kill(pid, sig) != 0) {
        perror("kill");
        return -1;
    }

    return 0;
}
