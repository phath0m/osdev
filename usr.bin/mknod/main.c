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


    int file_type = -1;

    switch (filetype[0]) {
        case 'p':
            file_type = S_IFIFO;
            break;
        case 'c':
            file_type = S_IFCHR;
            break;
    }

    if (file_type == -1) {
        fprintf(stderr, "mknod: invalid file type specified!\n");
        return -1;
    }

    
    int major = atoi(argv[3]);
    int minor = atoi(argv[4]);

    mknod(filepath, 0777 | file_type, makedev(major, minor));

    return 0;
}

