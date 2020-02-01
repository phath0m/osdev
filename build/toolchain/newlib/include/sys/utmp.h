#ifndef _SYS_UTMP_H
#define _SYS_UTMP_H

#include <time.h>

#define _PATH_UTMP      "/var/run/utmp"

/* I love you Solaris <3 */
#define _PATH_WTMP      "/var/adm/wtmp"
#define _PATH_LASTLOG   "/var/adm/lastlog"

#define UT_NAMESIZE     32
#define UT_LINESIZE     8
#define UT_HOSTSIZE     256

struct lastlog {
    time_t      ll_time;
    char        ll_line[UT_LINESIZE];
    char        ll_host[UT_HOSTSIZE];
};

struct utmp {
    char        ut_line[UT_LINESIZE];
    char        ut_name[UT_NAMESIZE];
    char        ut_host[UT_HOSTSIZE];
    time_t      ut_time;
};

#endif
