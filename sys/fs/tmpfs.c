#include <ds/dict.h>
#include <ds/list.h>
#include <ds/membuf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/string.h>
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
static int tmpfs_chmod(struct vfs_node *node, mode_t mode);
static int tmpfs_chown(struct vfs_node *node, uid_t owner, gid_t group);
static int tmpfs_creat(struct vfs_node *parent, struct vfs_node **child, const char *name, mode_t mode);
static int tmpfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int tmpfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
static int tmpfs_rmdir(struct vfs_node *parent, const char *dirname); 
static int tmpfs_mkdir(struct vfs_node *parent, const char *name, mode_t mode);
static int tmpfs_seek(struct vfs_node *node, uint64_t *pos, off_t off, int whence);
static int tmpfs_stat(struct vfs_node *node, struct stat *stat);
static int tmpfs_truncate(struct vfs_node *node, off_t length);
static int tmpfs_unlink(struct vfs_node *parent, const char *dirname);
static int tmpfs_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct file_ops tmpfs_file_ops = {
    .chmod      = tmpfs_chmod,
    .chown      = tmpfs_chown,
    .creat      = tmpfs_creat,
    .lookup     = tmpfs_lookup,
    .read       = tmpfs_read,
    .readdirent = tmpfs_readdirent,
    .rmdir      = tmpfs_rmdir,
    .mkdir      = tmpfs_mkdir,
    .seek       = tmpfs_seek,
    .stat       = tmpfs_stat,
    .truncate   = tmpfs_truncate,
    .unlink     = tmpfs_unlink,
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
tmpfs_chmod(struct vfs_node *node, mode_t mode)
{
    struct tmpfs_node *tmpfs_node = (struct tmpfs_node*)node->state;

    tmpfs_node->mode = mode;

    return 0;
}

static int
tmpfs_chown(struct vfs_node *node, uid_t owner, gid_t group)
{
    struct tmpfs_node *tmpfs_node = (struct tmpfs_node*)node->state;

    tmpfs_node->uid = owner;
    tmpfs_node->gid = group;

    return 0;
}

static int
tmpfs_creat(struct vfs_node *parent, struct vfs_node **child, const char *name, mode_t mode)
{
    struct tmpfs_node *node = tmpfs_node_new();

    node->content = membuf_new();
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
        
        node->state = child;
        node->mode = child->mode;
        node->uid = child->uid;
        node->gid = child->gid;    

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

    if (start >= MEMBUF_SIZE(file->content)) {
        return 0;
    }

    if (end > MEMBUF_SIZE(file->content)) {
        nbyte = end - start;
    }

    return membuf_read(file->content, buf, nbyte, pos);
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
            new_pos = MEMBUF_SIZE(file->content) - (off + 1);
            break;
        case SEEK_SET:
            new_pos = off;
            break;
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

    switch (file->type) {
        case DT_REG:
            stat->st_mode |= IFREG;
            break;
        case DT_DIR:
            stat->st_mode |= IFDIR;
            break;
    }

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
tmpfs_truncate(struct vfs_node *node, off_t length)
{
    struct tmpfs_node *tmpfs_node = (struct tmpfs_node*)node->state;

    membuf_clear(tmpfs_node->content);

    return 0;
}

static int
tmpfs_unlink(struct vfs_node *parent, const char *dirname)
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
tmpfs_write(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos)
{
    struct tmpfs_node *file = (struct tmpfs_node*)node->state;

    if (file->type != DT_REG) {
        return -(EISDIR);
    }

    membuf_write(file->content, buf, nbyte, pos);

    return nbyte;
}

__attribute__((constructor))
void
_init_tmpfs()
{
    register_filesystem("tmpfs", &tmpfs_ops);
}
