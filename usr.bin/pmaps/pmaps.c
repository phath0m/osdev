#include <stdio.h>
#include <stdlib.h>
//#include <sys/sysctl.h>
#include "../../sys/sys/sysctl.h"

int
main(int argc, char *argv[])
{
    int i;
    int nentries;
    int pid;
    size_t bufsize;
    int oid[4];
    struct kinfo_vmentry *cur;
    struct kinfo_vmentry *entries;

    if (argc != 2) {
        fprintf(stderr, "usage: pmaps PID\n");
        return -1;
    }

    pid = atoi(argv[1]);

    oid[0] = CTL_KERN;
    oid[1] = KERN_PROC;
    oid[2] = KERN_PROC_VMMAP;
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

    nentries = bufsize / sizeof(struct kinfo_vmentry);

    for (i = 0; i < nentries; i++) {
        cur = &entries[i];
        printf("%p-%p rwx\n", cur->start, cur->end);
    }

    free(entries);

    return 0;
}
