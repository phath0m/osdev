#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>

struct mount_options {
    char *          source;
    char *          target;
    char *          fstype;
    int             flags;
};

static void
usage()
{
    fputs("usage: mount [-t fstype] source target\n", stderr);
}

static int
parse_arguments(struct mount_options *options, int argc, char *argv[])
{
    int c;
    while ((c = getopt(argc, argv, ":t:")) != -1) {
        switch (c) {
            case 't':
                options->fstype = optarg;
                break;
            case '?':
                return -1;
        }
    }
    
    for (int i = 0; optind < argc; i++) {
        char *arg = argv[optind++];
        
        switch (i) {
            case 0:
                options->source = arg;
                break;
            case 1:
                options->target = arg;
                break;
        }
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    struct mount_options options;
    memset(&options, 0, sizeof(struct mount_options));

    if (parse_arguments(&options, argc, argv)) {
        usage();
        return -1;
    }

    int ret = mount(options.source, options.target, options.fstype, options.flags, NULL);

    if (ret != 0) {
        perror("mount");
        return -1;
    }

    return 0;
}
