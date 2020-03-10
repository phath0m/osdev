#ifndef _SYS_SYSCTL_H
#define _SYS_SYSCTL_H

#include <sys/types.h>

#define CTL_KERN            0x01

#define KERN_PROC           1
#define KERN_PROC_ALL       1

struct kinfo_proc {
    pid_t   pid;
    pid_t   ppid;
    uid_t   uid;
    uid_t   sid;
    uid_t   pgid;
    gid_t   gid;
    char    cmd[256];
    char    tty[32];
    time_t  stime;
};

int kern_sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen);
int sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen);

#endif
