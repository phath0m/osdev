#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    uid_t uid = getuid();
    gid_t gid = getgid();

    struct passwd *pwd = getpwuid(uid);
    struct group *grp = getgrgid(gid);

    char *username = "unknown";
    char *groupname = "unknown";

    if (pwd) {
        username = pwd->pw_name;
    }

    if (grp) {
        groupname = grp->gr_name;
    }

    printf("uid=%d(%s) gid=%d(%s)\n", uid, username, gid, groupname);

    return 0;
}

