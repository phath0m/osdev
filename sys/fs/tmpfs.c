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
#include <sys/cdev.h>
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
static int tmpfs_mount(struct vnode *parent, struct file *, struct vnode **root);
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
    struct tmpfs_node *node;
    
    node = calloc(1, sizeof(struct tmpfs_node));

    return node;
}

static int
tmpfs_chmod(struct vnode *vn, mode_t mode)
{
    struct tmpfs_node *tmpfs_node;
    
    tmpfs_node = vn->state;

    tmpfs_node->mode &= ~(0777);
    tmpfs_node->mode |= mode;
    vn->mode = tmpfs_node->mode;

    return 0;
}

static int
tmpfs_chown(struct vnode *vn, uid_t owner, gid_t group)
{
    struct tmpfs_node *tmpfs_node;
    
    tmpfs_node = vn->state;

    tmpfs_node->uid = owner;
    tmpfs_node->gid = group;

    return 0;
}

static int
tmpfs_creat(struct vnode *vn_parent, struct vnode **vn_child, const char *name, mode_t mode)
{
    struct tmpfs_node *parent_dir;
    struct tmpfs_node *node;
    
    node = tmpfs_node_new();

    node->content = membuf_new();
    node->gid = 0;
    node->mode = mode | S_IFREG;
    node->uid = 0;
    node->mtime = 0;

    parent_dir = vn_parent->state;

    dict_set(&parent_dir->children, name, node);

    return tmpfs_lookup(vn_parent, vn_child, name);
}

static int
tmpfs_lookup(struct vnode *vn_parent, struct vnode **vn_result, const char *name)
{
    struct tmpfs_node *dir;
    struct tmpfs_node *child;
    struct vnode *vn;

    dir = vn_parent->state;

    if (dict_get(&dir->children, name, (void**)&child)) {
        vn = vn_new(vn_parent, NULL, &tmpfs_file_ops);
        
        vn->state = child;
        vn->mode = child->mode;
        vn->uid = child->uid;
        vn->gid = child->gid;    
        vn->devno = child->devno;

        *vn_result = vn;

        return 0;
    }

    return -(ENOENT);
}

static int
tmpfs_mount(struct vnode *vn_parent, struct file *dev, struct vnode **vn_root)
{
    struct tmpfs_node *root;
    struct vnode *vn;

    vn = vn_new(vn_parent, NULL, &tmpfs_file_ops);
    root = tmpfs_node_new();

    root->mode = 0755 | S_IFDIR;
    vn->state = root;
    vn->mode = 0755 | S_IFDIR;

    *vn_root = vn;

    return 0;
}

static int
tmpfs_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos)
{
    uint64_t end;
    uint64_t start;
    
    struct tmpfs_node *file;

    file = node->state;
    
    start = pos;
    end = start + nbyte;

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
tmpfs_readdirent(struct vnode *vn, struct dirent *dirent, uint64_t entry)
{
    int i;
    int res;
    list_iter_t iter;
    
    char *name;

    struct tmpfs_node *dir;

    dir = vn->state;
    res = -1;

    dict_get_keys(&dir->children, &iter);


    for (i = 0; iter_move_next(&iter, (void**)&name); i++) {
        struct tmpfs_node *node;

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
tmpfs_rmdir(struct vnode *vn_parent, const char *dirname)
{
    struct tmpfs_node *parent_node;
    struct tmpfs_node *result;

    parent_node = vn_parent->state;

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
tmpfs_mkdir(struct vnode *vn_parent, const char *name, mode_t mode)
{
    struct tmpfs_node *node;
    struct tmpfs_node *parent_dir;

    node = tmpfs_node_new();

    node->gid = 0;
    node->mode = mode | S_IFDIR;
    node->uid = 0;
    node->mtime = 0;

    parent_dir = vn_parent->state;

    dict_set(&parent_dir->children, name, node);
    
    return 0;
}

static int
tmpfs_mknod(struct vnode *vn_parent, const char *name, mode_t mode, dev_t dev)
{
    struct tmpfs_node *node;
    struct tmpfs_node *parent_dir;

    parent_dir = vn_parent->state;

    node = tmpfs_node_new();

    node->gid = 0;
    node->mode = mode;
    node->uid = 0;
    node->mtime = 0;
    node->devno = dev;

    dict_set(&parent_dir->children, name, node);

    return 0;
}

static int
tmpfs_seek(struct vnode *vn, off_t *cur_pos, off_t off, int whence)
{
    off_t new_pos;
    struct tmpfs_node *file;

    file = vn->state;

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
tmpfs_stat(struct vnode *vn, struct stat *stat)
{
    struct tmpfs_node *file;

    file = vn->state;

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
tmpfs_truncate(struct vnode *vn, off_t length)
{
    struct tmpfs_node *tmpfs_node;
    
    tmpfs_node = vn->state;

    membuf_clear(tmpfs_node->content);

    return 0;
}

static int
tmpfs_unlink(struct vnode *vn_parent, const char *dirname)
{
    struct tmpfs_node *parent_node;
    struct tmpfs_node *result;

    parent_node = vn_parent->state;

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
tmpfs_utime(struct vnode *vn, struct timeval tv[2])
{
    struct tmpfs_node *file;

    file = vn->state;    

    file->atime = tv[0].tv_sec;
    file->mtime = tv[1].tv_sec;

    return 0;
}

static int
tmpfs_write(struct vnode *vn, const void *buf, size_t nbyte, uint64_t pos)
{
    struct tmpfs_node *file;
    
    file = vn->state;

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
