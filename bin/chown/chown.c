#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
    uid_t new_uid;
    gid_t new_gid;

    char *file;
    char *new_owner;
 
    struct stat buf;
    struct passwd *pwd;

    if (argc != 3) {
        fprintf(stderr, "usage: chown OWNER[:GROUP] FILE\n");
        return -1; 
    }

    new_owner = argv[1];
    file = argv[2];

    if (stat(file, &buf) != 0) {
        perror("chown");
        return -1;
    }

    new_uid = buf.st_uid;
    new_gid = buf.st_gid;

    pwd = NULL;

    if (!strrchr(new_owner, ':')) {
        pwd = getpwnam(new_owner);

        if (pwd) {
            new_uid = pwd->pw_uid;
        } else {
            fprintf(stderr, "chown: User does not exist\n");
            return -1;
        }
    }

    if (chown(file, new_uid, new_gid) != 0) {
        perror("chown");
        return -1;
    }

    return 0;
}