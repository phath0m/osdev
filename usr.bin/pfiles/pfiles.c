#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <elysium/sys/sysctl.h>

#define minor(n) (n & 0xFF)
#define major(n) ((n >> 8) & 0xFF)

static const char *
get_file_type(int mode)
{
    switch ((mode & S_IFMT)) {
        case S_IFDIR:
            return "S_IFDIR";
        case S_IFCHR:
            return "S_IFCHR";
        case S_IFIFO:
            return "S_IFIFO";
        case S_IFREG:
            return "S_IFREG";
        case S_IFSOCK:
            return "S_IFSOCK";
        default:
            return "UKNOWN";
    }
}

int
main(int argc, char *argv[])
{
    int i;
    int nentries;
    int pid;
    size_t bufsize;
    int oid[4];
    struct kinfo_ofile *cur;
    struct kinfo_ofile *entries;

    if (argc != 2) {
        fprintf(stderr, "usage: pfiles PID\n");
        return -1;
    }

    pid = atoi(argv[1]);

    oid[0] = CTL_KERN;
    oid[1] = KERN_PROC;
    oid[2] = KERN_PROC_FILES;
    oid[3] = pid;

    bufsize = 0;

    if (sysctl(oid, 4, NULL, &bufsize, NULL, 0)) {
        perror("sysctl");
        return -1;
    }

    entries = calloc(1, bufsize);

    if (sysctl(oid, 4, entries, &bufsize, NULL, 0)) {
        perror("sysctl");
        return -1;
    }

    nentries = bufsize / sizeof(struct kinfo_ofile);

    for (i = 0; i < nentries; i++) {
        cur = &entries[i];
        printf("%d: %s mode: %o dev: %d,%d\n", cur->fd,
                get_file_type(cur->type), (cur->type & ~S_IFMT),
                major(cur->dev), minor(cur->dev)
        );
        printf("    %s\n", cur->path);
    }

    free(entries);

    return 0;
}
