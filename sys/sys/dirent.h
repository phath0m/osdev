/*
 * dirent.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _ELYSIUM_SYS_DIRENT_H
#define _ELYSIUM_SYS_DIRENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/limits.h>
#include <sys/stat.h>

#define DT_REG      S_IFREG
/* There are no block devices */
#define DT_BLK      S_IFBLK
#define DT_CHR      S_IFCHR
#define DT_DIR      S_IFDIR
#define DT_FIFO     S_IFIFO

struct dirent {
    ino_t       inode;
    uint32_t    type;
    char        name[PATH_MAX];
};

#ifndef __KERNEL__
typedef struct {
    int             fd;
    struct dirent   buf;
} DIR;

DIR *           opendir(const char *);
int             closedir(DIR *);
struct dirent * readdir(DIR *);
void            rewinddir(DIR *);

#endif /* __KERNEL__  */
#ifdef __cplusplus
}
#endif
#endif /* _ELYSIUM_SYS_DIRENT_H */
