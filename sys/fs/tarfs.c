/*
 * tarfs.c - Tape archive based filesystem
 *
 * This was utilized in older versions of the kernel for the initial ramdisk
 * 
 * Currently, this isn't used for anything and will probably be removed
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
#include <sys/cdev.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vnode.h>

#define TAR_REG     '0'
#define TAR_SYMLINK '2'
#define TAR_DIR     '5'

struct ramfs_node;
struct tar_header;

static int ramfs_lookup(struct vnode *, struct vnode **, const char *);
static int ramfs_mount(struct vnode *, struct file *, struct vnode **);
static struct ramfs_node *ramfs_node_new();
static int ramfs_read(struct vnode *, void *, size_t, uint64_t);
static int ramfs_readdirent(struct vnode *, struct dirent *, uint64_t);
static int ramfs_seek(struct vnode *, off_t *, off_t, int);
static int ramfs_stat(struct vnode *, struct stat *);

struct vops ramfs_file_ops = {
    .lookup     = ramfs_lookup,
    .read       = ramfs_read,
    .readdirent = ramfs_readdirent,
    .seek       = ramfs_seek,
    .stat       = ramfs_stat
};

struct fs_ops ramfs_ops = {
    .mount = ramfs_mount,
};

struct ramfs_node {
    struct dict         children;
    struct tar_header * header;
    void *              data;
    size_t              size;
    uid_t               uid;
    gid_t               gid;
    uint32_t            mode;
    uint32_t            type;
    uint64_t            mtime;
};

struct ramfs_state {
    void *              archive;
    struct tar_node *   root;
};

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
};

static inline void
discard_trailing_slash(char *path)
{
    while (*path) {
        if (*path == '/' && path[1] == 0) {
            *path = 0;
            return;
        }
        path++;
    }
}

static struct ramfs_node *
ramfs_get_node(struct ramfs_node *root, const char *path)
{
    struct ramfs_node *parent;
    struct ramfs_node *child;

    char dir[PATH_MAX];
    char *nextdir;

    nextdir = NULL;
    parent = root;
    child = NULL;

    if (*path == 0 || !strcmp(path, "/")) {
        return root;
    }

    if (*path == '/') {
        path++;
    }

    do {
        nextdir = strrchr(path, '/');
        
        if (nextdir) {
            strncpy(dir, path, (nextdir - path) + 1);
            path = nextdir + 1;
        } else {
            strcpy(dir, path);
        }

        if (!dict_get(&parent->children, dir, (void**)&child)) {
            return NULL;
        }

        parent = child;
    } while (nextdir && *path);

    return child;
}

struct ramfs_node *
parse_tar_archive(const void *archive)
{
    int i;
    int size;
    char name[100];

    const char *basename;

    struct tar_header *header;
    struct ramfs_node *root;
    struct ramfs_node *node;
    struct ramfs_node *parent;

    root = ramfs_node_new();

    for (i = 0; ;i++) {
        header = (struct tar_header*)archive;

        if (!*header->name) {
            break;
        }

        strcpy(name, header->name + 1);

        discard_trailing_slash(name);

        node = ramfs_node_new();
        
        node->header = header;
        node->data = (void*)(archive + 512);
        node->gid = atoi(header->gid, 8);
        node->mode = atoi(header->mode, 8);
        node->uid = atoi(header->uid, 8);
        node->mtime = atoi(header->mtime, 8);

        size = atoi(header->size, 8);

        node->size = size;

        archive += ((size / 512) + 1) * 512;

        if (size % 512 != 0) {
            archive += 512;
        }

        if (i > 0) {
            basename = strchr(name, '/') + 1;
        
            *strchr(name, '/') = '\0';

            parent = (struct ramfs_node*)ramfs_get_node(root, name);

            dict_set(&parent->children, basename, node); 
            
            switch (header->typeflag) {
                case TAR_REG:
                    node->type = DT_REG;
                    break;
                case TAR_DIR:
                    node->type = DT_DIR;
                    break;
            }
        }
    }

    return root;
}

static struct ramfs_node *
ramfs_node_new()
{
    return (struct ramfs_node*)calloc(1, sizeof(struct ramfs_node));
}

static int
ramfs_lookup(struct vnode *parent, struct vnode **result, const char *name)
{
    struct ramfs_node *dir;
    struct ramfs_node *child;
    struct vnode *node;
    
    dir = (struct ramfs_node*)parent->state;

    if (dict_get(&dir->children, name, (void**)&child)) {
        node = vn_new(parent, NULL, &ramfs_file_ops);
        
        node->size = child->size;
        node->state = child;
        
        *result = node;

        return 0;
    }

    return -(ENOENT);
}

static int
ramfs_mount(struct vnode *parent, struct file *dev, struct vnode **root)
{
    extern void *start_initramfs;

    struct vnode *node;

    node = vn_new(parent, NULL, &ramfs_file_ops);

    node->state = parse_tar_archive(start_initramfs);
    
    *root = node;

    return 0;
}

static int
ramfs_read(struct vnode *node, void *buf, size_t nbyte, uint64_t pos)
{
    uint64_t end;
    uint64_t start;

    struct ramfs_node *file;

    file = (struct ramfs_node*)node->state;
    start = pos;
    end = start + nbyte;

    if (file->type != DT_REG) {
        return -(EISDIR);
    }

    if (start >= file->size) {
        return 0;
    }

    if (end > file->size) {
        nbyte = end - start;
    }

    memcpy(buf, file->data + pos, nbyte);

    return nbyte;
}

static int
ramfs_readdirent(struct vnode *vn, struct dirent *dirent, uint64_t entry)
{
    int i;
    int res;
    list_iter_t iter;

    char *name;
    struct ramfs_node *dir;
    struct ramfs_node *node;

    dir = (struct ramfs_node*)vn->state;

    dict_get_keys(&dir->children, &iter);

    res = -1;
    name = NULL;

    for (i = 0; iter_move_next(&iter, (void**)&name); i++) {
        if (i == entry && name && dict_get(&dir->children, name, (void**)&node)) {
            strncpy(dirent->name, name, PATH_MAX);
            dirent->type = node->type;
            res = 0;
            break;
        }
    }

    iter_close(&iter);

    return res;
}

static int
ramfs_seek(struct vnode *node, off_t *cur_pos, off_t off, int whence)
{
    off_t new_pos;

    struct ramfs_node *file;

    file = (struct ramfs_node*)node->state;

    switch (whence) {
        case SEEK_CUR:
            new_pos = *cur_pos + off;
            break;
        case SEEK_END:
            new_pos = file->size - (off + 1);
            break;
        case SEEK_SET:
            new_pos = off;
            break;
    }

    if (new_pos > file->size) {
        return -(ESPIPE);
    }

    if (new_pos < 0) {
        return -(ESPIPE);
    }

    *cur_pos = new_pos;

    return 0;
}

static int
ramfs_stat(struct vnode *node, struct stat *stat)
{
    struct ramfs_node *file;

    file = (struct ramfs_node*)node->state;

    stat->st_dev = (dev_t)0;
    stat->st_gid = file->gid;
    stat->st_mode = file->mode;
    stat->st_size = file->size;
    stat->st_uid = file->uid;
    stat->st_ino = (ino_t)file;
    stat->st_mtime = file->mtime;

    return 0;
}

void
init_tarfs()
{
    fs_register("tarfs", &ramfs_ops);
}
