#ifndef _AUTHLIB_H
#define _AUTHLIB_H

#include <time.h>

#define AUTH_UPDATE_UTMP        0x01
#define AUTH_UPDATE_WTMP        0x02
#define AUTH_UPDATE_LASTLOG     0x04
#define AUTH_UPDATE_BTMP        0x08

struct login {
    char        username[64];
    char        terminal[64];
    time_t      time;
    int         flags;
    int         successful;
    long int    utmp_offset;
};

int authlib_login(struct login *login, const char *username, const char *passwd, int flags);
int authlib_logout(struct login *login);

#endif
