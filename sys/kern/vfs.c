#include <stdlib.h>
#include <string.h>
#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/limits.h>
#include <sys/mount.h>
#include <sys/vfs.h>

// reminder: remove this
#include <stdio.h>

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

    INC_NODE_REF(node);

    file->node = node;
    file->refs = 1;
    return file;
}

int
vfs_close(struct file *file)
{
    if ((--file->refs) > 0) {
        return 0;
    }

    DEC_NODE_REF(file->node);

    free(file);

    return 0; 
}

struct file *
vfs_duplicate_file(struct file *file)
{
    if (!file) {
        return NULL;
    }

    struct file *new_file = (struct file*)malloc(sizeof(struct file));

    memcpy(new_file, file, sizeof(struct file));

    INC_NODE_REF(new_file->node);

    return new_file;
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

void
vfs_node_destroy(struct vfs_node *node)
{
    struct file_ops *ops = node->ops;

    if (ops && ops->close) {
        ops->close(node);
    }    

    /*
     * Note: this should actually attempt to free each item in children
     * That being said, this will not get called as the reference counting
     * is sort of broken atm
     */
    dict_clear(&node->children);
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
vfs_get_node(struct vfs_node *root, struct vfs_node *cwd, struct vfs_node **result, const char *path)
{
    if (*path == 0) {
        return -(ENOENT);
    }

    char dir[PATH_MAX];
    char *nextdir = NULL;

    struct vfs_node *parent = root;
    struct vfs_node *child = NULL;

    if (cwd && *path != '/') {
        parent = cwd;
    } else if (*path == '/') {
        path++;

        if (*path == 0) {
            *result = root;
            return 0;
        }
    }

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
        *result = child;

        return 0;
    }

    return -(ENOENT);
}

int
vfs_open(struct vfs_node *root, struct file **result, const char *path, int flags)
{
    return vfs_open_r(root, NULL, result, path, flags);
}

int
vfs_open_r(struct vfs_node *root, struct vfs_node *cwd, struct file **result, const char *path, int flags)
{
    struct vfs_node *child = NULL;

    if (vfs_get_node(root, cwd, &child, path) == 0) {
        bool will_write = (flags == O_RDWR || flags == O_WRONLY);
        //bool will_read = (flags == O_RDWR || flags == O_RDONLY);

        if ((child->mount_flags & MS_RDONLY) && will_write) {
            return -(EROFS);
        }

        struct file *file = file_new(child);
        
        file->flags = flags;

        *result = file;

        return 0;
    }

    return -(ENOENT);
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

    if (!strcmp(".", name)) {
        node = parent;
        res = 0;
    } else if (dict_get(&parent->children, name, (void**)&node)) {
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

            INC_NODE_REF(mount);

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

    if (ops->read) {
        int read = ops->read(node, buf, nbyte, file->position);
    
        file->position += read;

        return read;
    }

    return -(EPERM);
}

int
vfs_readdirent(struct file *file, struct dirent *dirent)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->readdirent) {
        int res = ops->readdirent(node, dirent, file->position);

        if (res == 0) {
            file->position++;
        }

        return res;
    }

    return -(EPERM);
}

int
vfs_seek(struct file *file, off_t off, int whence)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->seek) {
        return ops->seek(node, &file->position, off, whence);
    }

    return -(ESPIPE);
}

int
vfs_stat(struct file *file, struct stat *stat)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->stat) {
        return ops->stat(node, stat);
    }

    return -1;
}

uint64_t
vfs_tell(struct file *file)
{
    return file->position;
}

int
vfs_write(struct file *file, const char *buf, size_t nbyte)
{
    if (file->flags == O_RDONLY) {
        return -(EPERM);
    }

    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->write) {
        int written = ops->write(node, buf, nbyte, file->position);

        file->position += written;

        return written;
    }

    return -(EPERM);
}
