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
main(int argc, char *argv[])
{
    struct utsname buf;

    uname(&buf);

    int c;
    int flags = 0;

    /* I'm not sure why I have to do this, it isn't necessary on glibc but it is on newlib*/
    if (argc == 1) {
        optind = argc;
    }

    while (optind < argc) {
        if ((c = getopt(argc, argv, "asrvm")) != -1) {
            switch (c) {
                case 'a':
                    flags |= PRINT_ALL;
                    break;
                case 's':
                    flags |= PRINT_SYSNAME;
                    break;
                case 'r':
                    flags |= PRINT_RELEASE;
                    break;
                case 'v':
                    flags |= PRINT_VERSION;
                    break;
                case 'm':
                    flags |= PRINT_MACHINE;
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

    if (flags == 0) {
        flags = PRINT_SYSNAME;
    }

    if (flags & PRINT_SYSNAME) {
        fputs(buf.sysname, stdout);
        fputs(" ", stdout);
    }

    if (flags & PRINT_RELEASE) {
        fputs(buf.release, stdout);
        fputs(" ", stdout);
    }

    if (flags & PRINT_VERSION ) {
        fputs(buf.version, stdout);
        fputs(" ", stdout);
    }

    if (flags & PRINT_MACHINE) {
        fputs(buf.machine, stdout);
    }

    puts("");
    return 0;
}

