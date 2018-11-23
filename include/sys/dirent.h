#ifndef SYS_DIRENT_H
#define SYS_DIRENT_H

#include <sys/limits.h>

#define DT_REG      0x01
/* There are no block devices */
#define DT_CHR      0x02
#define DT_DIR      0x04

struct dirent {
    ino_t   inode;
    uint8_t type;
    char    name[PATH_MAX];
};

#endif
