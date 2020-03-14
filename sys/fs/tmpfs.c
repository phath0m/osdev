/*
 * tmpfs.c - Temporary filesystem implementation
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
#include <ds/dict.h>
#include <ds/list.h>
#include <ds/membuf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vnode.h>
// delete me
#include <sys/systm.h>

#define TAR_REG     '0'
#define TAR_SYMLINK '2'
#define TAR_DIR     '5'

struct tmpfs_node;
struct tar_header;

static int tmpfs_lookup(struct vnode *parent, struct vnode **result, const char *name);
static int tmpfs_mount(struct vnode *parent, struct cdev *dev, struct vnode **root);
//static struct tmpfs_node *tmpfs_node_new();
static int tmpfs_chmod(struct vnode *node, mode_t mode);
static int tmpfs_chown(struct vnode *node, uid_t owner, gid_t group);
static int tmpfs_creat(struct vnode *parent, struct vnode **child, const char *name, mode_t mode);
static int tmpfs_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos);
static int tmpfs_readdirent(struct vnode *node, struct dirent *dirent, uint64_t entry);
static int tmpfs_rmdir(struct vnode *parent, const char *dirname); 
static int tmpfs_mkdir(struct vnode *parent, const char *name, mode_t mode);
static int tmpfs_mknod(struct vnode *parent, const char *name, mode_t mode, dev_t dev);
static int tmpfs_seek(struct vnode *node, off_t *pos, off_t off, int whence);
static int tmpfs_stat(struct vnode *node, struct stat *stat);
static int tmpfs_truncate(struct vnode *node, off_t length);
static int tmpfs_unlink(struct vnode *parent, const char *dirname);
static int tmpfs_utime(struct vnode *node, struct timeval tv[2]);
static int tmpfs_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos);

struct vops tmpfs_file_ops = {
    .chmod      = tmpfs_chmod,
    .chown      = tmpfs_chown,
    .creat      = tmpfs_creat,
    .lookup     = tmpfs_lookup,
    .read       = tmpfs_read,
    .readdirent = tmpfs_readdirent,
    .rmdir      = tmpfs_rmdir,
    .mkdir      = tmpfs_mkdir,
    .mknod      = tmpfs_mknod,
    .seek       = tmpfs_seek,
    .stat       = tmpfs_stat,
    .truncate   = tmpfs_truncate,
    .unlink     = tmpfs_unlink,
    .utimes     = tmpfs_utime,
    .write      = tmpfs_write
};

struct fs_ops tmpfs_ops = {
    .mount = tmpfs_mount,
};

struct tmpfs_node {
    struct dict         children;
    struct membuf *     content;
    uid_t               uid;
    gid_t               gid;
    dev_t               devno;
    uint16_t            mode;
    time_t              atime;
    time_t              mtime;
};

static struct tmpfs_node *
tmpfs_node_new()
{
    struct tmpfs_node *node = (struct tmpfs_node*)calloc(0, sizeof(struct tmpfs_node));

    return node;
}

static int
tmpfs_chmod(struct vnode *node, mode_t mode)
{
    struct tmpfs_node *tmpfs_node = (struct tmpfs_node*)node->state;

    tmpfs_node->mode = mode;

    return 0;
}

static int
tmpfs_chown(struct vnode *node, uid_t owner, gid_t group)
{
    struct tmpfs_node *tmpfs_node = (struct tmpfs_node*)node->state;

    tmpfs_node->uid = owner;
    tmpfs_node->gid = group;

    return 0;
}

static int
tmpfs_creat(struct vnode *parent, struct vnode **child, const char *name, mode_t mode)
{
    struct tmpfs_node *node = tmpfs_node_new();

    node->content = membuf_new();
    node->gid = 0;
    node->mode = mode | S_IFREG;
    node->uid = 0;
    node->mtime = 0;

    struct tmpfs_node *parent_dir = (struct tmpfs_node*)parent->state;

    dict_set(&parent_dir->children, name, node);

    return tmpfs_lookup(parent, child, name);
}

static int
tmpfs_lookup(struct vnode *parent, struct vnode **result, const char *name)
{
    struct tmpfs_node *dir = (struct tmpfs_node*)parent->state;

    struct tmpfs_node *child;

    if (dict_get(&dir->children, name, (void**)&child)) {
        struct vnode *node = vn_new(parent, NULL, &tmpfs_file_ops);
        
        node->state = child;
        node->mode = child->mode;
        node->uid = child->uid;
        node->gid = child->gid;    
        node->devno = child->devno;

        *result = node;

        return 0;
    }

    return -(ENOENT);
}

static int
tmpfs_mount(struct vnode *parent, struct cdev *dev, struct vnode **root)
{
    struct vnode *node = vn_new(parent, dev, &tmpfs_file_ops);

    struct tmpfs_node *root_node = tmpfs_node_new();
    root_node->mode = 0755;

    node->state = root_node;
    node->mode = 0755;

    *root = node;

    return 0;
}

static int
tmpfs_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;
    
    uint64_t start = pos;
    uint64_t end = start + nbyte;

    if (!S_ISREG(file->mode)) {
        return -(EISDIR);
    }

    if (start >= MEMBUF_SIZE(file->content)) {
        return 0;
    }

    if (end > MEMBUF_SIZE(file->content)) {
        nbyte = MEMBUF_SIZE(file->content) - start;
    }
    
    return membuf_read(file->content, buf, nbyte, pos);
}

static int
tmpfs_readdirent(struct vnode *node, struct dirent *dirent, uint64_t entry)
{
    struct tmpfs_node *dir = (struct tmpfs_node*)node->state;

    list_iter_t iter;

    dict_get_keys(&dir->children, &iter);

    char *name = NULL;

    int res = -1;

    for (int i = 0; iter_move_next(&iter, (void**)&name); i++) {
        struct tmpfs_node *node = NULL;

        if (i == entry && name && dict_get(&dir->children, name, (void**)&node)) {
            strncpy(dirent->name, name, PATH_MAX);
            dirent->type = (node->mode & S_IFMT);
            res = 0;
            break;
        }
    }

    iter_close(&iter);

    return res;
}

static int
tmpfs_rmdir(struct vnode *parent, const char *dirname)
{
    struct tmpfs_node *parent_node = (struct tmpfs_node*)parent->state;
    struct tmpfs_node *result;

    if (!dict_get(&parent_node->children, dirname, (void**)&result)) {
        return -(ENOENT);
    }

    if (dict_count(&result->children) != 0) {
        return -(ENOTEMPTY);
    }

    dict_remove(&parent_node->children, dirname);

    return 0;
}

static int
tmpfs_mkdir(struct vnode *parent, const char *name, mode_t mode)
{
    struct tmpfs_node *node = tmpfs_node_new();

    node->gid = 0;
    node->mode = mode | S_IFDIR;
    node->uid = 0;
    node->mtime = 0;

    struct tmpfs_node *parent_dir = (struct tmpfs_node*)parent->state;

    dict_set(&parent_dir->children, name, node);
    
    return 0;
}

static int
tmpfs_mknod(struct vnode *parent, const char *name, mode_t mode, dev_t dev)
{
    struct tmpfs_node *node = tmpfs_node_new();

    node->gid = 0;
    node->mode = mode;
    node->uid = 0;
    node->mtime = 0;
    node->devno = dev;

    struct tmpfs_node *parent_dir = (struct tmpfs_node*)parent->state;

    dict_set(&parent_dir->children, name, node);

    return 0;
}

static int
tmpfs_seek(struct vnode *node, off_t *cur_pos, off_t off, int whence)
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;

    off_t new_pos;

    switch (whence) {
        case SEEK_CUR:
            new_pos = *cur_pos + off;
            break;
        case SEEK_END:
            new_pos = MEMBUF_SIZE(file->content) - off;
            break;
        case SEEK_SET:
            new_pos = off;
            break;
    }

    if (new_pos < 0) {
        return -(ESPIPE);
    }

    *cur_pos = new_pos;

    return new_pos;
}

static int
tmpfs_stat(struct vnode *node, struct stat *stat)
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;

    stat->st_dev = (dev_t)0;
    stat->st_gid = file->gid;
    stat->st_mode = file->mode;

    if (file->content) {
        stat->st_size = MEMBUF_SIZE(file->content);
    } else {
        stat->st_size = 0;
    }

    stat->st_uid = file->uid;
    stat->st_ino = (ino_t)file;
    stat->st_mtime = file->mtime;
    return 0;
}

static int
tmpfs_truncate(struct vnode *node, off_t length)
{
    struct tmpfs_node *tmpfs_node = (struct tmpfs_node*)node->state;

    membuf_clear(tmpfs_node->content);

    return 0;
}

static int
tmpfs_unlink(struct vnode *parent, const char *dirname)
{
    struct tmpfs_node *parent_node = (struct tmpfs_node*)parent->state;
    struct tmpfs_node *result;

    if (!dict_get(&parent_node->children, dirname, (void**)&result)) {
        return -(ENOENT);
    }

    if (dict_count(&result->children) != 0) {
        return -(ENOTEMPTY);
    }

    dict_remove(&parent_node->children, dirname);

    dict_clear(&result->children);
    membuf_destroy(result->content);
    free(result);

    return 0;
}

static int
tmpfs_utime(struct vnode *node, struct timeval tv[2])
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;

    file->atime = tv[0].tv_sec;
    file->mtime = tv[1].tv_sec;

    return 0;
}

static int
tmpfs_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;

    if (!S_ISREG(file->mode)) {
        return -(EISDIR);
    }

    membuf_write(file->content, buf, nbyte, pos);

    return nbyte;
}

void
tmpfs_init()
{
    fs_register("tmpfs", &tmpfs_ops);
}
