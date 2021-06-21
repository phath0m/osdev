/*
 * mount.c - Connects filesystem implementations to the virtual filesystem
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
#include <ds/list.h>
#include <sys/cdev.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/string.h>
#include <sys/vnode.h>

struct list fs_list;

static struct filesystem *
getfsbyname(const char *name)
{
    list_iter_t iter;

    struct filesystem *fs;
    struct filesystem *res;

    list_get_iter(&fs_list, &iter);

    res = NULL;

    while (iter_move_next(&iter, (void**)&fs)) {
        if (strcmp(name, fs->name) == 0) {
            res = fs;
            break;
        }
    }
    
    iter_close(&iter);

    return res;
}

int
fs_open(struct cdev *dev, struct vnode **root, const char *fsname, int flags)
{
    int res;
    struct filesystem *fs;
    struct vnode *vn;

    fs = getfsbyname(fsname);

    if (fs) {
        vn = NULL;

        res = fs->ops->mount(NULL, dev, &vn);

        if (vn) {
            vn->mount_flags = flags;
            *root = vn;
        }

        return res;
    }

    return -1;
}

void
fs_register(char *name, struct fs_ops *ops)
{
    struct filesystem *fs;
    
    fs = malloc(sizeof(struct filesystem));

    fs->name = name;
    fs->ops = ops;

    list_append(&fs_list, fs);
}

int
fs_mount(struct vnode *root, struct cdev *dev, const char *fsname, const char *path, int flags)
{
    struct vnode *mount;
    struct file *file;
    struct vnode *mount_point;

    if (fs_open(dev, &mount, fsname, flags) == 0) {
        if (vfs_open(current_proc, &file, path, O_RDONLY) == 0) {
            if (fop_getvn(file, &mount_point) == 0) { 
                mount_point->ismount = true;
                mount_point->mount = mount;
            
                VN_INC_REF(mount);
            }
            return 0;
        }
    }

    return -1;
}

bool
fs_probe(struct cdev *dev, const char *fsname, int uuid_length, const uint8_t *uuid)
{
    struct filesystem *fs;
    
    fs = getfsbyname(fsname);

    if (!fs) {
        return false;

    }

    return fs->ops->probe(dev, uuid_length, uuid);
}
