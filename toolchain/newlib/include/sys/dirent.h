#ifndef DIRENT_H
#define DIRENT_H

#include <stdint.h>

#define DT_REG      0x01
/* There are no block devices */
#define DT_BLK      0x08
#define DT_CHR      0x02
#define DT_DIR      0x04


struct dirent {
    uint32_t    d_ino;
    uint8_t     d_type;
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
