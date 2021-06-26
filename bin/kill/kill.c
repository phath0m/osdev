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
    int pid;
    int sig;

    if (argc < 2) {
        usage();
        return -1;
    }

    sig = SIGTERM;

    if (!strncmp(argv[1], "-", 1)) {        
        const char *opt;

        if (argc != 3) {
            usage();
            return -1;
        }

        opt = &argv[1][1];

        sig = atoi(opt);
    }

    pid = atoi(argv[argc - 1]);

    if (kill(pid, sig) != 0) {
        perror("kill");
        return -1;
    }

    return 0;
}
