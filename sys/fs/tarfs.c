#include <ds/dict.h>
#include <ds/list.h>
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

struct ramfs_node;
struct tar_header;

static int ramfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name);
static int ramfs_mount(struct vfs_node *parent, struct device *dev, struct vfs_node **root);
static struct ramfs_node *ramfs_node_new();
static int ramfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
static int ramfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
static int ramfs_seek(struct vfs_node *node, uint64_t *pos, off_t off, int whence);
static int ramfs_stat(struct vfs_node *node, struct stat *stat);

struct file_ops ramfs_file_ops = {
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
    uint8_t             mode;
    uint8_t             type;
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
    struct ramfs_node *parent = root;
    struct ramfs_node *child = NULL;

    char dir[PATH_MAX];
    char *nextdir = NULL;

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
    struct ramfs_node *root = ramfs_node_new();

    for (int i = 0; ;i++) {
        struct tar_header *header = (struct tar_header*)archive;

        if (!*header->name) {
            break;
        }

        char name[100];

        strcpy(name, header->name + 1);

        discard_trailing_slash(name);

        struct ramfs_node *node = ramfs_node_new();
        
        node->header = header;
        node->data = (void*)(archive + 512);
        node->gid = atoi(header->gid, 8);
        node->mode = atoi(header->mode, 8);
        node->uid = atoi(header->uid, 8);
        node->mtime = atoi(header->mtime, 8);

        int size = atoi(header->size, 8);

        node->size = size;

        archive += ((size / 512) + 1) * 512;

        if (size % 512 != 0) {
            archive += 512;
        }

        if (i > 0) {
            const char *basename = strchr(name, '/') + 1;
        
            *strchr(name, '/') = '\0';

            struct ramfs_node *parent = (struct ramfs_node*)ramfs_get_node(root, name);

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
    struct ramfs_node *node = (struct ramfs_node*)calloc(0, sizeof(struct ramfs_node));

    return node;
}

static int
ramfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name)
{
    struct ramfs_node *dir = (struct ramfs_node*)parent->state;

    struct ramfs_node *child;

    if (dict_get(&dir->children, name, (void**)&child)) {
        struct vfs_node *node = vfs_node_new(parent, NULL, &ramfs_file_ops);
        
        node->size = child->size;
        node->state = child;
        
        *result = node;

        return 0;
    }

    return -(ENOENT);
}

static int
ramfs_mount(struct vfs_node *parent, struct device *dev, struct vfs_node **root)
{
    extern void *start_initramfs;

    struct vfs_node *node = vfs_node_new(parent, dev, &ramfs_file_ops);

    node->state = parse_tar_archive(start_initramfs);
    
    *root = node;

    return 0;
}

static int
ramfs_read(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos)
{
    struct ramfs_node *file = (struct ramfs_node*)node->state;
    
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
ramfs_readdirent(struct vfs_node *node, struct dirent *dirent, uint64_t entry)
{
    struct ramfs_node *dir = (struct ramfs_node*)node->state;

    list_iter_t iter;

    dict_get_keys(&dir->children, &iter);

    char *name = NULL;

    int res = -1;

    for (int i = 0; iter_move_next(&iter, (void**)&name); i++) {
        struct ramfs_node *node = NULL;

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
ramfs_seek(struct vfs_node *node, uint64_t *cur_pos, off_t off, int whence)
{
    struct ramfs_node *file = (struct ramfs_node*)node->state;

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
ramfs_stat(struct vfs_node *node, struct stat *stat)
{
    struct ramfs_node *file = (struct ramfs_node*)node->state;

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
_init_ramfs()
{
    register_filesystem("initramfs", &ramfs_ops);
}
