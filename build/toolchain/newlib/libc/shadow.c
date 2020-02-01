#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shadow.h>
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
parse_shadow_line(struct spwd *spwd, char *buf, size_t bufsize, FILE *fp)
{
    if (fgets(buf, bufsize, fp) <= 0) {
        return -1;
    }

    if (count_char(buf, bufsize, ':') != 8) {
        return -1;
    }

    char *cur = buf;

    buf[strcspn(buf, "\n")] = 0;    

    spwd->sp_namp = strsep(&cur, ":");
    spwd->sp_pwdp = strsep(&cur, ":");
    spwd->sp_lstchg = atoi(strsep(&cur, ":"));
    spwd->sp_min = atoi(strsep(&cur, ":"));
    spwd->sp_max = atoi(strsep(&cur, ":"));
    spwd->sp_warn = atoi(strsep(&cur, ":"));
    spwd->sp_inact = atoi(strsep(&cur, ":"));
    spwd->sp_expire = atoi(strsep(&cur, ":"));

    return 0;
}

int
getspnam_r(const char *name, struct spwd *spwd, char *buf, size_t bufsize, struct spwd **result)
{
    FILE *fp = fopen("/etc/shadow", "r");

    if (!fp) {
        return -1;
    }

    int ret = -1;

    while (parse_shadow_line(spwd, buf, bufsize, fp) == 0) {
        if (strcmp(spwd->sp_namp, name) == 0) {
            ret = 0;
            break;
        }
    }

    fclose(fp);

    if (ret == 0 && result) {
        *result = spwd;
    }

    return ret;
}

struct spwd *
getspnam(const char *name)
{
    static struct spwd spwd;
    static char buf[1024];

    if (getspnam_r(name, &spwd, buf, 1024, NULL) == 0) {
        return &spwd;
    }

    return NULL;
}

static FILE *shadow_fp;

struct spwd *
getspent(void)
{
    static struct spwd spwd;

    if (!shadow_fp) {
        shadow_fp = fopen("/etc/shadow", "r");
    }

    if (!shadow_fp) {
        return NULL;
    }

    char buf[1024];

    if (parse_shadow_line(&spwd, buf, 1024, shadow_fp) != 0) {
        return NULL;
    }

    return &spwd;
}

void
endspent(void)
{
    fclose(shadow_fp);
    shadow_fp = NULL;
}

void
setspent(void)
{
    fseek(shadow_fp, 0, SEEK_SET);
}
