#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/dirent.h>
#include <sys/syscalls.h>

DIR *
opendir(const char *dirname)
{
    int fd = open(dirname, O_RDONLY);

    if (fd >= 0) {
        DIR *dirp = (DIR*)malloc(sizeof(DIR));

        dirp->fd = fd;

        return dirp;
    }

    return NULL;
}

int
closedir(DIR *dirp)
{
    close(dirp->fd);

    free(dirp);
}

struct dirent *
readdir(DIR *dirp)
{
    int ret;

    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_READDIR), "b"(dirp->fd), "c"(&dirp->buf));

    if (ret == 0) {
        return &dirp->buf;
    }

    return NULL;
}

void rewinddir(DIR *dirp)
{
    lseek(dirp->fd, 0, SEEK_SET);
}

