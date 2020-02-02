#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/string.h>
#include <sys/unistd.h>
#include <sys/vfs.h>

// delete me
#include <sys/systm.h>

int vfs_file_count = 0;

static bool
split_path(char *path, char **directory, char **file)
{
    int last_slash = -1;

    for (int i = 0; i < PATH_MAX && path[i]; i++) {
        if (path[i] == '/') last_slash = i;
    }

    if (last_slash == -1) {
        *file = path;
        return false;
    }

    path[last_slash] = 0;

    if (directory) {
        if (*path == 0) {
            *directory = "/";
        } else {
            *directory = path;
        }
    }
    if (file) *file = &path[last_slash + 1];

    return true;
}

static bool
can_read(struct vfs_node *node, struct cred *creds)
{
    if (creds->uid == 0) {
        return true;
    }

    if (node->uid == creds->uid && (node->mode & S_IRUSR)) {
        return true;
    }

    return (node->mode & S_IROTH);
}

static bool
can_write(struct vfs_node *node, struct cred *creds)
{
    if (creds->uid == 0) {
        return true;
    }

    if (node->uid == creds->uid && (node->mode & S_IWUSR)) {
        return true;
    }

    return (node->mode & S_IWOTH);
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
fops_access(struct proc *proc, const char *path, int mode)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    bool will_write = mode & W_OK;
    bool will_read = mode & R_OK;

    struct vfs_node *child = NULL;

    if (vfs_get_node(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }

    if (will_write && (child->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    if (will_write && !can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    if (will_read && !can_read(child, &proc->creds)) {
        return -(EACCES);
    }

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
        ops->close(file->node, file);
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

    new_file->refs = 1;

    INC_NODE_REF(new_file->node);

    vfs_file_count++;

    struct file_ops *ops = new_file->node->ops;

    if (ops && ops->duplicate) {
        ops->duplicate(new_file->node, new_file);
    }

    return new_file;
}

int
fops_open(struct proc *proc, struct file **result, const char *path, int flags)
{
    return fops_open_r(proc, result, path, flags);
}

int
fops_open_r(struct proc *proc, struct file **result, const char *path, int flags)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    struct vfs_node *child = NULL;

    if (vfs_get_node(root, cwd, &child, path) == 0) {
        bool will_write = (flags & O_WRONLY);
        bool will_read = true; // this is a hack. 

        if ((child->mount_flags & MS_RDONLY) && will_write) {
            return -(EROFS);
        }

        if (will_write && !can_write(child, &proc->creds)) {
            return -(EACCES);
        }

        if (will_read && !can_read(child, &proc->creds)) {
            return -(EACCES);
        }

        if ((child->mode & S_IFIFO)) {
            child = fifo_open(child, flags);
        }

        struct file *file = file_new(child);
        
        file->flags = flags;

        *result = file;

        return 0;
    }

    if ((flags & O_CREAT)) {
        return fops_creat(proc, result, path, 0700);
    }

    return -(ENOENT);
}

int
fops_chmod(struct proc *proc, const char *path, mode_t mode)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    struct vfs_node *child = NULL;

    if (vfs_get_node(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }

    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    struct file_ops *ops = child->ops;

    if (ops && ops->chmod) {
        return ops->chmod(child, mode);
    }

    return -(ENOTSUP);
}

int
fops_chown(struct proc *proc, const char *path, uid_t owner, gid_t group)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    struct vfs_node *child = NULL;

    if (vfs_get_node(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }

    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    struct file_ops *ops = child->ops;

    if (ops && ops->chown) {
        return ops->chown(child, owner, group);
    }

    return -(ENOTSUP);
}

int
fops_creat(struct proc *proc, struct file **result, const char *path, mode_t mode)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    char *filename;
    char *parent_path;
    struct vfs_node *parent;

    if (!split_path(path_buf, &parent_path, &filename)) {
        parent = cwd;
    } else if (vfs_get_node(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    struct file_ops *ops = parent->ops;
    struct vfs_node *child;

    if (!ops || !ops->creat) {
        return -(ENOTSUP);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
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
fops_fchown(struct file *file, uid_t owner, gid_t group)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->chown) {
        return ops->chown(node, owner, group);
    }

    return -(ENOTSUP);
}

int
fops_ftruncate(struct file *file, off_t length)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->truncate) {
        return ops->truncate(node, length);
    }

    return -(ENOTSUP);
}

int
fops_ioctl(struct file *file, uint64_t request, void *arg)
{
    struct vfs_node *node = file->node;
    struct file_ops *ops = node->ops;

    if (ops->ioctl) {
        return ops->ioctl(node, request, arg);
    }

    return -(ENOTSUP);
}

int
fops_mkdir(struct proc *proc, const char *path, mode_t mode)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);
    
    char *dirname;
    char *parent_path;
    struct vfs_node *parent;

    if (!split_path(path_buf, &parent_path, &dirname)) {
        parent = cwd;
    } else if (vfs_get_node(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    struct file_ops *ops = parent->ops;

    if (ops && ops->mkdir) {
        return ops->mkdir(parent, dirname, mode);
    }

    return -(ENOTSUP);
}

int
fops_mknod(struct proc *proc, const char *path, mode_t mode, dev_t dev)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    char *nodename;
    char *parent_path;
    struct vfs_node *parent;

    if (!split_path(path_buf, &parent_path, &nodename)) {
        parent = cwd;
    } else if (vfs_get_node(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    struct file_ops *ops = parent->ops;

    if (ops && ops->mknod) {
        return ops->mknod(parent, nodename, mode, dev);
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
fops_rmdir(struct proc *proc, const char *path)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    char *dirname;
    char *parent_path;
    struct vfs_node *parent;

    if (!split_path(path_buf, &parent_path, &dirname)) {
        parent = cwd;
    } else if (vfs_get_node(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
       return -(EROFS);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    struct file_ops *ops = parent->ops;

    if (ops && ops->rmdir) {
        int res = ops->rmdir(parent, dirname);
        
        struct vfs_node *child;

        if (res == 0 && dict_get(&parent->children, dirname, (void**)&child)) {
            dict_remove(&parent->children, dirname);
            DEC_NODE_REF(child);
        }

        return res;
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

    return -(ENOTSUP);
}

uint64_t
fops_tell(struct file *file)
{
    return file->position;
}

int
fops_truncate(struct proc *proc, const char *path, off_t length)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    struct vfs_node *child = NULL;

    if (vfs_get_node(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }
    
    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    struct file_ops *ops = child->ops;

    if (ops && ops->truncate) {
        return ops->truncate(child, length);
    }

    return -(ENOTSUP);
}

int
fops_unlink(struct proc *proc, const char *path)
{
    struct vfs_node *root = proc->root;
    struct vfs_node *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    char *filename;
    char *parent_path;
    struct vfs_node *parent;

    if (!split_path(path_buf, &parent_path, &filename)) {
        parent = cwd;
    } else if (vfs_get_node(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    struct file_ops *ops = parent->ops;

    if (ops && ops->unlink) {
        int res = ops->unlink(parent, filename);
        
        struct vfs_node *child; 

        if (res == 0 && dict_get(&parent->children, filename, (void**)&child)) {
            dict_remove(&parent->children, filename);
            DEC_NODE_REF(child);
        }
        
        return res;
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
