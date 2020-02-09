#include <errno.h>
#include <grp.h>
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
parse_group_line(struct group *gwd, char *buf, size_t bufsize, FILE *fp)
{
    if (fgets(buf, bufsize, fp) <= 0) {
        return -1;
    }

    if (count_char(buf, bufsize, ':') != 3) {
        return -1;
    }

    char *cur = buf;

    buf[strcspn(buf, "\n")] = 0;    

    gwd->gr_name = strsep(&cur, ":");
    strsep(&cur, ":");
    gwd->gr_gid = atoi(strsep(&cur, ":"));
    gwd->gr_mem = NULL;

    return 0;
}

int
getgrgid_r(gid_t gid, struct group *group, char *buf, size_t bufsize, struct group **result)
{
    FILE *fp = fopen("/etc/group", "r");

    if (!fp) {
        return -1;
    }

    int ret = -1;

    while (parse_group_line(group, buf, bufsize, fp) == 0) {
        if (group->gr_gid == gid) {
            ret = 0;
            break;
        }
    }

    fclose(fp);

    if (ret == 0 && result) {
        *result = group;
    }

    return ret;
}

struct group *
getgrgid(gid_t gid)
{
    static struct group group;
    static char buf[1024];

    if (getgrgid_r(gid, &group, buf, 1024, NULL) == 0) {
        return &group;
    }

    return NULL;
}

int
getgrnam_r(const char *name, struct group *group, char *buf, size_t bufsize, struct group **result)
{
    FILE *fp = fopen("/etc/group", "r");

    if (!fp) {
        return -1;
    }

    int ret = -1;

    while (parse_group_line(group, buf, bufsize, fp) == 0) {
        if (strcmp(group->gr_name, name) == 0) {
            ret = 0;
            break;
        }
    }

    fclose(fp);

    if (ret == 0 && result) {
        *result = group;
    }

    return ret;
}

struct group *
getgrnam(const char *name)
{
    static struct group group;
    static char buf[1024];

    if (getgrnam_r(name, &group, buf, 1024, NULL) == 0) {
        return &group;
    }

    return NULL;
}

static FILE *group_fp;

struct group *
getgrent(void)
{
    static struct group group;

    if (!group_fp) {
        group_fp = fopen("/etc/group", "r");
    }

    if (!group_fp) {
        return NULL;
    }

    char buf[1024];

    if (parse_group_line(&group, buf, 1024, group_fp) != 0) {
        return NULL;
    }

    return &group;
}

void
endgrent(void)
{
    fclose(group_fp);
    group_fp = NULL;
}

void
setgrent(void)
{
    fseek(group_fp, 0, SEEK_SET);
}
