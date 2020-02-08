#ifndef DIRENT_H
#define DIRENT_H

#include <stdint.h>
#include <sys/stat.h>

#define DT_REG      S_IFREG
/* There are no block devices */
#define DT_BLK      S_IFBLK
#define DT_CHR      S_IFCHR
#define DT_DIR      S_IFDIR
#define DT_LNK      S_IFLNK
#define DT_FIFO     S_IFIFO

struct dirent {
    uint32_t    d_ino;
    uint32_t    d_type;
    char        d_name[256];
};

typedef struct {
    int             fd;
    struct dirent   buf;
} DIR;

DIR * opendir(const char *dirname);
int closedir(DIR *dirp);
struct dirent *readdir(DIR *dirp);
void rewinddir(DIR *dirp);

#endif
