#ifndef SYS_DIRENT_H
#define SYS_DIRENT_H

#include <sys/limits.h>
#include <sys/stat.h>

#define DT_REG      S_IFREG
/* There are no block devices */
#define DT_CHR      S_IFCHR
#define DT_DIR      S_IFDIR
#define DT_FIFO     S_IFIFO

struct dirent {
    ino_t       inode;
    uint32_t    type;
    char        name[PATH_MAX];
};

#endif
