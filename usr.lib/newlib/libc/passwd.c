#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static int
count_char(const char *str, int len, char ch)
{
    int i;
    int ret = 0;

    for (i = 0; i < len && str[i]; i++) {
        if (str[i] == ch) {
            ret++;
        }
    }        

    return ret;
}

static int
parse_passwd_line(struct passwd *pwd, char *buf, size_t bufsize, FILE *fp)
{
    if (fgets(buf, bufsize, fp) <= 0) {
        return -1;
    }

    if (count_char(buf, bufsize, ':') != 6) {
        return -1;
    }

    char *cur = buf;

    buf[strcspn(buf, "\n")] = 0;    

    pwd->pw_name = strsep(&cur, ":");
    pwd->pw_passwd = strsep(&cur, ":");
    pwd->pw_uid = atoi(strsep(&cur, ":"));
    pwd->pw_gid = atoi(strsep(&cur, ":"));
    pwd->pw_gecos = strsep(&cur, ":");
    pwd->pw_dir = strsep(&cur, ":");
    pwd->pw_shell = strsep(&cur, ":");
    
    return 0;
}

int
getpwuid_r(uid_t uid, struct passwd *passwd, char *buf, size_t bufsize, struct passwd **result)
{
    FILE *fp = fopen("/etc/passwd", "r");

    if (!fp) {
        return -1;
    }

    int ret = -1;

    while (parse_passwd_line(passwd, buf, bufsize, fp) == 0) {
        if (passwd->pw_uid == uid) {
            ret = 0;
            break;
        }
    }

    fclose(fp);

    if (ret == 0 && result) {
        *result = passwd;
    }

    return ret;
}

struct passwd *
getpwuid(uid_t uid)
{
    static struct passwd passwd;
    static char buf[1024];

    if (getpwuid_r(uid, &passwd, buf, 1024, NULL) == 0) {
        return &passwd;
    }

    return NULL;
}

int
getpwnam_r(const char *name, struct passwd *passwd, char *buf, size_t bufsize, struct passwd **result)
{
    FILE *fp = fopen("/etc/passwd", "r");

    if (!fp) {
        return -1;
    }

    int ret = -1;

    while (parse_passwd_line(passwd, buf, bufsize, fp) == 0) {
        if (strcmp(passwd->pw_name, name) == 0) {
            ret = 0;
            break;
        }
    }

    fclose(fp);

    if (ret == 0 && result) {
        *result = passwd;
    }

    return ret;
}

struct passwd *
getpwnam(const char *name)
{
    static struct passwd passwd;
    static char buf[1024];

    if (getpwnam_r(name, &passwd, buf, 1024, NULL) == 0) {
        return &passwd;
    }

    return NULL;
}

static FILE *passwd_fp;

struct passwd *
getpwent(void)
{
    static struct passwd passwd;

    if (!passwd_fp) {
        passwd_fp = fopen("/etc/passwd", "r");
    }

    if (!passwd_fp) {
        return NULL;
    }

    char buf[1024];

    if (parse_passwd_line(&passwd, buf, 1024, passwd_fp) != 0) {
        return NULL;
    }

    return &passwd;
}

void
endpwent(void)
{
    fclose(passwd_fp);
    passwd_fp = NULL;
}

void
setpwent(void)
{
    fseek(passwd_fp, 0, SEEK_SET);
}
