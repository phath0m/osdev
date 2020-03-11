#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

static void
usage()
{
    fprintf(stderr, "usage: mknod FILE TYPE [MAJOR MINOR]\n");
}

int
main (int argc, char *argv[])
{
    
    if (argc < 3) {
        usage();
        return -1;
    }


    char *filepath = argv[1];
    char *filetype = argv[2];

    
    if (strlen(filetype) != 1) {
        fprintf(stderr, "mknod: invalid file type specified!\n");
        return -1;
    }


    mode_t mode = -1;

    switch (filetype[0]) {
        case 'p':
            mode = S_IFIFO;
            break;
        case 'c':
            mode = S_IFCHR;
            break;
    }

    if (mode == -1) {
        fprintf(stderr, "mknod: invalid file type specified!\n");
        return -1;
    }

    if (mode != S_IFIFO && argc != 5) {
        fprintf(stderr, "mknod: major and minor must be specified if creating a "
                        "device file!\n");
        return -1;
    }

    dev_t dev = 0;   
    mode |= 0777;

    if (!(mode & S_IFIFO)) {
        int major = atoi(argv[3]);
        int minor = atoi(argv[4]);
        dev = makedev(major, minor);
    }

    if (mknod(filepath, mode, dev) != 0) {
        perror("mknod");
        return -1;
    }

    return 0;
}

