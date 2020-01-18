#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

struct utsname {
    char    sysname[24];
    char    nodename[128];
    char    release[64];
    char    version[64];
    char    machine[64];
};

#endif
