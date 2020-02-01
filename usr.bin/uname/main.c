#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

#define PRINT_SYSNAME   0x01
#define PRINT_NODENAME  0x02
#define PRINT_RELEASE   0x04
#define PRINT_VERSION   0x08
#define PRINT_MACHINE   0x10
#define PRINT_ALL       0x1F

static void
usage()
{
    fprintf(stderr, "usage: uname [-asrvm]\n");
}

int
main (int argc, char *argv[])
{
    struct utsname buf;

    uname(&buf);

    int c;
    int mask = 0;

    /* I'm not sure why I have to do this, it isn't nessicary on glibc but it is on newlib*/
    if (argc == 1) {
        optind = argc;
    }

    while (optind < argc) {
        if ((c = getopt(argc, argv, "asrvm")) != -1) {
            switch (c) {
                case 'a':
                    mask |= PRINT_ALL;
                    break;
                case 's':
                    mask |= PRINT_SYSNAME;
                    break;
                case 'r':
                    mask |= PRINT_RELEASE;
                    break;
                case 'v':
                    mask |= PRINT_VERSION;
                    break;
                case 'm':
                    mask |= PRINT_MACHINE;
                    break;
                case '?':
                    usage();
                    return -1;
            }
        } else {
            usage();
            return -1;
        }
    }

    if (mask == 0) {
        mask = PRINT_SYSNAME;
    }

    if (mask & PRINT_SYSNAME) {
        printf("%s ", buf.sysname);
    }

    if (mask & PRINT_RELEASE) {
        printf("%s ", buf.release);
    }

    if (mask & PRINT_VERSION ) {
        printf("%s ", buf.version);
    }

    if (mask & PRINT_MACHINE) {
        printf("%s ", buf.machine);
    }

    puts("");

    return 0;
}

