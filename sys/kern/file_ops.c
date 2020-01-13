#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/limits.h>
#include <sys/mount.h>
#include <sys/vfs.h>

// reminder: remove this
#include <stdio.h>

int vfs_file_count = 0;

static bool
split_path(char *path, char **directory, char **file)
{
    int last_slash = 0;

    for (int i = 0; i < PATH_MAX && path[i]; i++) {
        if (path[i] == '/') last_slash = i;
    }

    path[last_slash] = 0;

    if (directory) *directory = path;
    if (file) *file = &path[last_slash + 1];

    return true;
}

struct file *
file_new(struct vfs_node *node)
{
    struct file *file = (struct file*)calloc(0, sizeof(struct file));

    INC_NODE_REF(node);

    file->node = node;
    file->refs = 1;
    vfs_file_count++;

    return file;
}

int
fops_access(struct vfs_node *root, struct vfs_node *cwd, const char *path, int mode)
{
    bool will_write = mode & W_OK;
    //bool will_read = mode & R_OK;

    struct vfs_node *child = NULL;

    if (vfs_get_node(root, cwd, &child, path) == 0) {

        if ((child->mount_flags & MS_RDONLY) && will_write) {
            return -(EROFS);
        }
    } else {
        return -(ENOENT);
    }

    // TODO: implement actual checks here...

    return 0;
}

int
fops_close(struct file *file)
{
    if ((--file->refs) > 0) {
        return 0;
    }

    DEC_NODE_REF(file->node);

    struct file_ops *ops = file->node->ops;
    
    if (ops->close) {
        ops->close(file->node);
    }

    vfs_file_count--;
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

    vfs_file_count++;

    return new_file;
}

int
fops_open(struct vfs_node *root, struct file **result, const char *path, int flags)
{
    return fops_open_r(root, NULL, result, path, flags);
}

int
fops_open_r(struct vfs_node *root, struct vfs_node *cwd, struct file **result, const char *path, int flags)
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

    if ((flags & O_CREAT)) {
        return fops_creat(root, cwd, result, path, 0700);
    }

    return -(ENOENT);
}

int
fops_creat(struct vfs_node *root, struct vfs_node *cwd, struct file **result, const char *path, mode_t mode)
{
    char parent_path[PATH_MAX+1];

    strncpy(parent_path, path, PATH_MAX);

    char *filename;

    split_path(parent_path, NULL, &filename);

    struct vfs_node *parent;

    if (vfs_get_node(root, cwd, &parent, parent_path) == 0) {

        if ((parent->mount_flags & MS_RDONLY)) {
            return -(EROFS);
        }
    } else {
        return -(ENOENT);
    }

    struct file_ops *ops = parent->ops;
    struct vfs_node *child;

    if (!ops || !ops->creat) {
        return -(ENOTSUP);
    }
    
    int res = ops->creat(parent, &child, filename, mode);

    if (res != 0) {
        return res;
    }    

    struct file *file = file_new(child);

    file->flags = O_WRONLY | O_CREAT;

    *result = file;

    return 0;
}

int
fops_fchmod(struct file *file, mode_t mode)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->chmod) {
        return ops->chmod(node, mode);
    }

    return -(ENOTSUP);
}

int
fops_mkdir(struct vfs_node *root, struct vfs_node *cwd, const char *path, mode_t mode)
{
    char parent_path[PATH_MAX+1];

    strncpy(parent_path, path, PATH_MAX);
    
    char *dirname;

    split_path(parent_path, NULL, &dirname);

    struct vfs_node *parent;

    if (vfs_get_node(root, cwd, &parent, parent_path) == 0) {
        if ((parent->mount_flags & MS_RDONLY)) {
            return -(EROFS);
        }
    } else {
        return -(ENOENT);
    }

    struct file_ops *ops = parent->ops;

    if (ops && ops->mkdir) {
        return ops->mkdir(parent, dirname, mode);
    }

    return -(ENOTSUP);
}

int
fops_read(struct file *file, char *buf, size_t nbyte)
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
fops_readdirent(struct file *file, struct dirent *dirent)
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
fops_rmdir(struct vfs_node *root, struct vfs_node *cwd, const char *path)
{
    char parent_path[PATH_MAX+1];

    strncpy(parent_path, path, PATH_MAX);

    char *dirname;

    split_path(parent_path, NULL, &dirname);

    struct vfs_node *parent;

    if (vfs_get_node(root, cwd, &parent, parent_path) == 0) {
        if ((parent->mount_flags & MS_RDONLY)) {
            return -(EROFS);
        }
    } else {
        return -(ENOENT);
    }

    struct file_ops *ops = parent->ops;

    if (ops && ops->rmdir) {
        return ops->rmdir(parent, dirname);
    }

    return -(ENOTSUP);
}

int
fops_seek(struct file *file, off_t off, int whence)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->seek) {
        return ops->seek(node, &file->position, off, whence);
    }

    return -(ESPIPE);
}

int
fops_stat(struct file *file, struct stat *stat)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->stat) {
        return ops->stat(node, stat);
    }

    return -1;
}

uint64_t
fops_tell(struct file *file)
{
    return file->position;
}

int
fops_unlink(struct vfs_node *root, struct vfs_node *cwd, const char *path)
{
    char parent_path[PATH_MAX+1];

    strncpy(parent_path, path, PATH_MAX);

    char *filename;

    split_path(parent_path, NULL, &filename);

    struct vfs_node *parent;

    if (vfs_get_node(root, cwd, &parent, parent_path) == 0) {
        if ((parent->mount_flags & MS_RDONLY)) {
            return -(EROFS);
        }
    } else {
        return -(ENOENT);
    }

    struct file_ops *ops = parent->ops;

    if (ops && ops->unlink) {
        return ops->unlink(parent, filename);
    }

    return -(ENOTSUP);
}

int
fops_write(struct file *file, const char *buf, size_t nbyte)
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
