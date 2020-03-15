/*
 * stat.h
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
#ifndef _SYS_STAT_H
#define _SYS_STAT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define S_IRWXU     0000700
#define S_IRUSR     0000400
#define S_IWUSR     0000200
#define S_IXUSR     0000100

#define S_IRWXG     0000070
#define S_IRGRP     0000040
#define S_IWGRP     0000020
#define S_IXGRP     0000010
#define S_IRWXO     0000007
#define S_IROTH     0000004
#define S_IWOTH     0000002
#define S_IXOTH     0000001

#define S_IFMT      0170000 /* file type mask */

#define S_IFDIR     0040000 /* directory */
#define S_IFCHR     0020000 /* character special */
#define S_IFBLK     0060000 /* block special */
#define S_IFREG     0100000 /* regular */
#define S_IFIFO     0010000 /* named pipe */
#define S_IFSOCK    0140000 /* unix socket */
#define S_ISBLK(m)  (((m)&S_IFMT) == S_IFBLK)
#define S_ISCHR(m)  (((m)&S_IFMT) == S_IFCHR)
#define S_ISDIR(m)  (((m)&S_IFMT) == S_IFDIR)
#define S_ISFIFO(m) (((m)&S_IFMT) == S_IFIFO)
#define S_ISREG(m)  (((m)&S_IFMT) == S_IFREG)

struct stat {
    dev_t       st_dev;
    ino_t       st_ino;
    mode_t      st_mode;
    nlink_t     st_nlink;
    uid_t       st_uid;
    gid_t       st_gid;
    dev_t       st_rdev;
    off_t       st_size;
    time_t      st_atime;
    long        st_spare1;
    time_t      st_mtime;
    long        st_spare2;
    time_t      st_ctime;
    long        st_spare3;
    int         st_blksize;
    int         st_blocks;
    long        st_spare4[2];
};

#ifdef __cplusplus
}
#endif
#endif /* _SYS_STAT_H */
