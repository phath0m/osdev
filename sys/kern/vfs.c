#include <rtl/malloc.h>
#include <rtl/list.h>
#include <rtl/string.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/vfs.h>

// reminder: remove this
#include <rtl/printf.h>

struct list fs_list;

static struct filesystem *
getfsbyname(const char *name)
{
    list_iter_t iter;

    list_get_iter(&fs_list, &iter);

    struct filesystem *fs;
    struct filesystem *res = NULL;

    while (iter_move_next(&iter, (void**)&fs)) {
        if (strcmp(name, fs->name) == 0) {
            res = fs;
            break;
        }
    }
    
    iter_close(&iter);

    return res;
}

struct file *
file_new(struct vfs_node *node)
{
    struct file *file = (struct file*)calloc(0, sizeof(struct file));

    file->node = node;

    return file;
}

int
vfs_openfs(struct device *dev, struct vfs_node **root, const char *fsname, int flags)
{
    struct filesystem *fs = getfsbyname(fsname);

    if (fs) {
        struct vfs_node *node = NULL;

        int result = fs->ops->mount(dev, &node);

        if (node) {
            node->mount_flags = flags;
            *root = node;
        }

        return result;
    }

    return -1;
}

struct vfs_mount *
vfs_mount_new(struct device *dev, struct filesystem *fs, uint64_t flags)
{
    struct vfs_mount *mount = (struct vfs_mount*)calloc(0, sizeof(struct vfs_mount));

    mount->device = dev;
    mount->filesystem = fs;
    mount->flags = flags;

    return mount;
}

struct vfs_node *
vfs_node_new(struct device *dev, struct file_ops *ops)
{
    struct vfs_node *node = (struct vfs_node*)calloc(0, sizeof(struct vfs_node));
    
    node->device = dev;
    node->ops = ops;

    return node;
}

int
vfs_open(struct vfs_node *root, struct file **result, const char *path, int flags)
{
    if (*path == 0) {
        return ENOENT;
    }

    if (*path == '/') {
        path++;
    }

    char dir[PATH_MAX];
    char *nextdir = NULL;

    struct vfs_node *parent = root;
    struct vfs_node *child = NULL;

    do {
        nextdir = strrchr(path, '/');

        if (nextdir) {
            strncpy(dir, path, (nextdir - path) + 1);
            path = nextdir + 1;
        } else {
            strcpy(dir, path);
        }

        int res = vfs_lookup(parent, &child, dir);
        
        if (res != 0) {
            return res;
        }

        parent = child;

    } while (nextdir && *path);
    
    if (child) {
        
        bool will_write = (flags == O_RDWR || flags == O_WRONLY);
        //bool will_read = (flags == O_RDWR || flags == O_RDONLY);

        if ((child->mount_flags & MS_RDONLY) && will_write) {
            return EPERM;
        }

        struct file *file = file_new(child);
        
        file->flags = flags;

        INC_NODE_REF(child);

        *result = file;

        return 0;
    }

    return -1;
}

void
register_filesystem(char *name, struct fs_ops *ops)
{
    struct filesystem *fs = (struct filesystem*)malloc(sizeof(struct filesystem));

    fs->name = name;
    fs->ops = ops;

    list_append(&fs_list, fs);
}

int
vfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name)
{
    struct vfs_node *node = NULL;

    int res = ENOENT;

    if (dict_get(&parent->children, name, (void**)&node)) {
        res = 0;
    } else if (parent->ops->lookup(parent, &node, name) == 0) {
        dict_set(&parent->children, name, node);
        
        node->mount_flags = parent->mount_flags;

        INC_NODE_REF(node);

        res = 0;
    }

    if (node && node->ismount) {
        *result = node->mount;
    } else if (node) {
        *result = node;
    }

    return res;
}

int
vfs_mount(struct vfs_node *root, struct device *dev, const char *fsname, const char *path, int flags)
{
    struct vfs_node *mount;
    struct file *file;

    if (vfs_openfs(dev, &mount, fsname, flags) == 0) {
        if (vfs_open(root, &file, path, O_RDONLY) == 0) {
            struct vfs_node *mount_point = file->node;
            
            mount_point->ismount = true;
            mount_point->mount = mount;

            return 0;
        }
    }

    return -1;
}

int
vfs_read(struct file *file, char *buf, size_t nbyte)
{
    if (file->flags == O_WRONLY) {
        return -(EPERM);
    }

    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    int read = ops->read(node, buf, nbyte, file->position);
    
    file->position += read;

    return read;
}

bool
vfs_readdirent(struct file *file, struct dirent *dirent)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    bool succ = ops->readdirent(node, dirent, file->position);
    
    if (succ) {
        file->position++;
    }

    return succ;
}

int
vfs_write(struct file *file, const char *buf, size_t nbyte)
{
    if (file->flags == O_RDONLY) {
        return -(EPERM);
    }

    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    int written = ops->write(node, buf, nbyte, file->position);

    file->position += written;

    return written;
}
