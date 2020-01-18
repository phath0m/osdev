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
parse_passwd_line(struct passwd *pwd, FILE *fp)
{
    static char line[1024];
    
    if (fgets(line, 1024, fp) <= 0) {
        return -1;
    }

    if (count_char(line, 1024, ':') != 6) {
        return -1;
    }

    line[strcspn(line, "\n")] = 0;    

    pwd->pw_name = strtok(line, ":");
    pwd->pw_passwd = strtok(NULL, ":");
    pwd->pw_uid = atoi(strtok(NULL, ":"));
    pwd->pw_gid = atoi(strtok(NULL, ":"));
    pwd->pw_gecos = strtok(NULL, ":");
    pwd->pw_dir = strtok(NULL, ":");
    pwd->pw_shell = strtok(NULL, ":");
    
    return NULL;
}

struct passwd *
getpwuid(uid_t uid)
{
    static struct passwd passwd;

    FILE *fp = fopen("/etc/passwd", "r");

    if (!fp) {
        return NULL;
    }

    int found = 0;

    while (parse_passwd_line(&passwd, fp) == 0) {
        if (passwd.pw_uid == uid) {
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (found) {
        return &passwd;
    }

    return NULL;
}

struct passwd *
getpwnam(const char * name)
{
    static struct passwd passwd;

    FILE *fp = fopen("/etc/passwd", "r");

    if (!fp) {
        return NULL;
    }

    int found = 0;

    while (parse_passwd_line(&passwd, fp) == 0) {
        if (strcmp(passwd.pw_name, name) == 0) {
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (found) {
        return &passwd;
    }

    return NULL; 
}
