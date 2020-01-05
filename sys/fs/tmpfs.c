#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ds/dict.h>
#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>

#define TAR_REG     '0'
#define TAR_SYMLINK '2'
#define TAR_DIR     '5'

struct tmpfs_node;
struct tar_header;

static int tmpfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name);
static int tmpfs_mount(struct device *dev, struct vfs_node **root);
//static struct tmpfs_node *tmpfs_node_new();
static int tmpfs_creat(struct vfs_node *parent, struct vfs_node **child, const char *name, mode_t mode);
static int tmpfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int tmpfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
static int tmpfs_rmdir(struct vfs_node *parent, const char *dirname); 
static int tmpfs_mkdir(struct vfs_node *parent, const char *name, mode_t mode);
static int tmpfs_seek(struct vfs_node *node, uint64_t *pos, off_t off, int whence);
static int tmpfs_stat(struct vfs_node *node, struct stat *stat);

struct file_ops tmpfs_file_ops = {
    .creat      = tmpfs_creat,
    .lookup     = tmpfs_lookup,
    .read       = tmpfs_read,
    .readdirent = tmpfs_readdirent,
    .rmdir      = tmpfs_rmdir,
    .mkdir      = tmpfs_mkdir,
    .seek       = tmpfs_seek,
    .stat       = tmpfs_stat
};

struct fs_ops tmpfs_ops = {
    .mount = tmpfs_mount,
};

struct tmpfs_node {
    struct dict         children;
    void *              data;
    size_t              size;
    uid_t               uid;
    gid_t               gid;
    uint16_t            mode;
    uint8_t             type;
    uint64_t            mtime;
};

static struct tmpfs_node *
tmpfs_node_new()
{
    struct tmpfs_node *node = (struct tmpfs_node*)calloc(0, sizeof(struct tmpfs_node));

    return node;
}

static int
tmpfs_creat(struct vfs_node *parent, struct vfs_node **child, const char *name, mode_t mode)
{
    struct tmpfs_node *node = tmpfs_node_new();

    node->data = NULL;
    node->gid = 0;
    node->mode = mode;
    node->uid = 0;
    node->mtime = 0;
    node->type = DT_REG;

    struct tmpfs_node *parent_dir = (struct tmpfs_node*)parent->state;

    dict_set(&parent_dir->children, name, node);

    return tmpfs_lookup(parent, child, name);
}

static int
tmpfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name)
{
    struct tmpfs_node *dir = (struct tmpfs_node*)parent->state;

    struct tmpfs_node *child;

    if (dict_get(&dir->children, name, (void**)&child)) {
        struct vfs_node *node = vfs_node_new(NULL, &tmpfs_file_ops);
        
        node->size = child->size;
        node->state = child;
        
        *result = node;

        return 0;
    }

    return -(ENOENT);
}

static int
tmpfs_mount(struct device *dev, struct vfs_node **root)
{
    struct vfs_node *node = vfs_node_new(dev, &tmpfs_file_ops);

    node->state = tmpfs_node_new();

    *root = node;

    return 0;
}

static int
tmpfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;
    
    uint64_t start = pos;
    uint64_t end = start + nbyte;

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
tmpfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry)
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
            dirent->type = node->type;
            res = 0;
            break;
        }
    }

    iter_close(&iter);

    return res;
}

static int
tmpfs_rmdir(struct vfs_node *parent, const char *dirname)
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
tmpfs_mkdir(struct vfs_node *parent, const char *name, mode_t mode)
{
    struct tmpfs_node *node = tmpfs_node_new();

    node->data = NULL;
    node->gid = 0;
    node->mode = mode;
    node->uid = 0;
    node->mtime = 0;
    node->type = DT_DIR;

    struct tmpfs_node *parent_dir = (struct tmpfs_node*)parent->state;

    dict_set(&parent_dir->children, name, node);
    
    return 0;
}

static int
tmpfs_seek(struct vfs_node *node, uint64_t *cur_pos, off_t off, int whence)
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;

    off_t new_pos;

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
tmpfs_stat(struct vfs_node *node, struct stat *stat)
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;

    stat->st_dev = (dev_t)0;
    stat->st_gid = file->gid;
    stat->st_mode = file->mode;
    stat->st_size = file->size;
    stat->st_uid = file->uid;
    stat->st_ino = (ino_t)file;
    stat->st_mtime = file->mtime;
    return 0;
}

__attribute__((constructor))
void
_init_tmpfs()
{
    register_filesystem("tmpfs", &tmpfs_ops);
}
