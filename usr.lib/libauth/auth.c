#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>
#include <sys/shadow.h>
#include "authlib.h"
#include "sha1.h"


static int
log_utmp(struct login *login)
{
    FILE *fp = fopen("/var/run/utmp", "a");

    if (!fp) {
        return -1;
    }
   
    struct utmp utmp;

    memset(&utmp, 0, sizeof(utmp));
    
    utmp.ut_time = login->time;

    char *tty_name = ttyname(0);

    strncpy(utmp.ut_name, login->username, UT_NAMESIZE);
    strncpy(utmp.ut_line, basename(tty_name), UT_LINESIZE);
 
    login->utmp_offset = ftell(fp);
    
    fwrite(&utmp, 1, sizeof(utmp), fp);

    fclose(fp);
    
    return 0;
}

static int
hexstr_to_bytes(const char *hex, unsigned char *buf, size_t nbytes)
{
    if (strlen(hex) / 2 != nbytes) {
        return -1;
    }

    for (int i = 0; i < nbytes; i++) {
        char octet_str[3];
        strncpy(octet_str, &hex[i*2], 2);
        octet_str[2] = 0;
        buf[i] = strtol(octet_str, NULL, 16);
    }

    return 0;
}

int
authlib_login(struct login *login, const char *user, const char *passwd, int flags)
{
    login->time = time(NULL);
    login->flags = flags;
    strncpy(login->username, user, sizeof(login->username));

    struct spwd *spwd = getspnam(user);

    if (!spwd) {
        return -1;
    }

    unsigned char hash_buf[20];

    if (hexstr_to_bytes(spwd->sp_pwdp, hash_buf, sizeof(hash_buf)) != 0) {
        return -1;
    }

    unsigned char result_buf[20];

    SHA1_CTX ctx;

    SHA1Init(&ctx);
    SHA1Update(&ctx, (unsigned char*)passwd, strlen(passwd)); 
    SHA1Final(result_buf, &ctx);

    int res = memcmp(hash_buf, result_buf, 20);
    
    if (res == 0) {
        if ((flags & AUTH_UPDATE_UTMP)) {
            log_utmp(login);
        }
    }

    return res;
}

int
authlib_logout(struct login *login)
{
    FILE *fp = fopen("/var/run/utmp", "w");

    if (!fp) {
        return -1;
    }

    fseek(fp, login->utmp_offset, SEEK_SET);

    struct utmp utmp;

    memset(&utmp, 0, sizeof(utmp));

    fwrite(&utmp, 1, sizeof(utmp), fp);

    fclose(fp);

    return 0;
}
