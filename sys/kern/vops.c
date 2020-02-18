#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/string.h>
#include <sys/unistd.h>
#include <sys/vnode.h>

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
can_read(struct vnode *node, struct cred *creds)
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
can_write(struct vnode *node, struct cred *creds)
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
file_new(struct vnode *node)
{
    struct file *file = (struct file*)calloc(0, sizeof(struct file));

    VN_INC_REF(node);

    file->node = node;
    file->refs = 1;
    vfs_file_count++;

    return file;
}

int
vops_access(struct proc *proc, const char *path, int mode)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    bool will_write = mode & W_OK;
    bool will_read = mode & R_OK;

    struct vnode *child = NULL;

    if (vn_open(root, cwd, &child, path) != 0) {
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
vops_close(struct file *file)
{
    if ((--file->refs) > 0) {
        return 0;
    }

    if (!file->node) {
        return 0;
    }

    VN_DEC_REF(file->node);

    struct vops *ops = file->node->ops;
    
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

    VN_INC_REF(new_file->node);

    vfs_file_count++;

    struct vops *ops = new_file->node->ops;

    if (ops && ops->duplicate) {
        ops->duplicate(new_file->node, new_file);
    }

    return new_file;
}

int
vops_open(struct proc *proc, struct file **result, const char *path, int flags)
{
    return vops_open_r(proc, result, path, flags);
}

int
vops_open_r(struct proc *proc, struct file **result, const char *path, int flags)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    struct vnode *child = NULL;

    if (vn_open(root, cwd, &child, path) == 0) {
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

        if ((child->mode & S_IFCHR)) {
            child = device_file_open(child, child->devno);
        }

        struct file *file = file_new(child);
        
        file->flags = flags;

        *result = file;

        return 0;
    }

    if ((flags & O_CREAT)) {
        return vops_creat(proc, result, path, 0700);
    }

    return -(ENOENT);
}

int
vops_chmod(struct proc *proc, const char *path, mode_t mode)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    struct vnode *child = NULL;

    if (vn_open(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }

    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    struct vops *ops = child->ops;

    if (ops && ops->chmod) {
        return ops->chmod(child, mode);
    }

    return -(ENOTSUP);
}

int
vops_chown(struct proc *proc, const char *path, uid_t owner, gid_t group)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    struct vnode *child = NULL;

    if (vn_open(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }

    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    struct vops *ops = child->ops;

    if (ops && ops->chown) {
        return ops->chown(child, owner, group);
    }

    return -(ENOTSUP);
}

int
vops_creat(struct proc *proc, struct file **result, const char *path, mode_t mode)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    char *filename;
    char *parent_path;
    struct vnode *parent;

    if (!split_path(path_buf, &parent_path, &filename)) {
        parent = cwd;
    } else if (vn_open(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    struct vops *ops = parent->ops;
    struct vnode *child;

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
vops_fchmod(struct file *file, mode_t mode)
{
    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->chmod) {
        return ops->chmod(node, mode);
    }

    return -(ENOTSUP);
}

int
vops_fchown(struct file *file, uid_t owner, gid_t group)
{
    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->chown) {
        return ops->chown(node, owner, group);
    }

    return -(ENOTSUP);
}

int
vops_ftruncate(struct file *file, off_t length)
{
    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->truncate) {
        return ops->truncate(node, length);
    }

    return -(ENOTSUP);
}

int
vops_ioctl(struct file *file, uint64_t request, void *arg)
{
    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->ioctl) {
        return ops->ioctl(node, request, arg);
    }

    return -(ENOTSUP);
}

int
vops_mkdir(struct proc *proc, const char *path, mode_t mode)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);
    
    char *dirname;
    char *parent_path;
    struct vnode *parent;

    if (!split_path(path_buf, &parent_path, &dirname)) {
        parent = cwd;
    } else if (vn_open(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    struct vops *ops = parent->ops;

    if (ops && ops->mkdir) {
        return ops->mkdir(parent, dirname, mode);
    }

    return -(ENOTSUP);
}

int
vops_mknod(struct proc *proc, const char *path, mode_t mode, dev_t dev)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    char *nodename;
    char *parent_path;
    struct vnode *parent;

    if (!split_path(path_buf, &parent_path, &nodename)) {
        parent = cwd;
    } else if (vn_open(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    struct vops *ops = parent->ops;

    if (ops && ops->mknod) {
        return ops->mknod(parent, nodename, mode, dev);
    }

    return -(ENOTSUP);
}

int
vops_mmap(struct file *file, uintptr_t addr, size_t size, int prot, off_t offset)
{
    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->mmap) {
        return ops->mmap(node, addr, size, prot, offset);
    }

    return -(ENOTSUP);
}

int
vops_read(struct file *file, char *buf, size_t nbyte)
{
    if (file->flags == O_WRONLY) {
        return -(EPERM);
    }

    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->read) {
        int read = ops->read(node, buf, nbyte, file->position);
    
        file->position += read;

        return read;
    }

    return -(EPERM);
}

int
vops_readdirent(struct file *file, struct dirent *dirent)
{
    struct vnode *node = file->node;
    struct vops *ops = node->ops;

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
vops_rmdir(struct proc *proc, const char *path)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    char *dirname;
    char *parent_path;
    struct vnode *parent;

    if (!split_path(path_buf, &parent_path, &dirname)) {
        parent = cwd;
    } else if (vn_open(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
       return -(EROFS);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    struct vops *ops = parent->ops;

    if (ops && ops->rmdir) {
        int res = ops->rmdir(parent, dirname);
        
        struct vnode *child;

        if (res == 0 && dict_get(&parent->children, dirname, (void**)&child)) {
            dict_remove(&parent->children, dirname);
            VN_DEC_REF(child);
        }

        return res;
    }

    return -(ENOTSUP);
}

int
vops_seek(struct file *file, off_t off, int whence)
{
    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->seek) {
        return ops->seek(node, &file->position, off, whence);
    }

    return -(ESPIPE);
}

int
vops_stat(struct file *file, struct stat *stat)
{
    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->stat) {
        return ops->stat(node, stat);
    }

    return -(ENOTSUP);
}

off_t
vops_tell(struct file *file)
{
    return file->position;
}

int
vops_truncate(struct proc *proc, const char *path, off_t length)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    struct vnode *child = NULL;

    if (vn_open(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }
    
    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    struct vops *ops = child->ops;

    if (ops && ops->truncate) {
        return ops->truncate(child, length);
    }

    return -(ENOTSUP);
}

int
vops_unlink(struct proc *proc, const char *path)
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    char *filename;
    char *parent_path;
    struct vnode *parent;

    if (!split_path(path_buf, &parent_path, &filename)) {
        parent = cwd;
    } else if (vn_open(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    struct vops *ops = parent->ops;

    if (ops && ops->unlink) {
        int res = ops->unlink(parent, filename);
        
        struct vnode *child; 

        if (res == 0 && dict_get(&parent->children, filename, (void**)&child)) {
            dict_remove(&parent->children, filename);
            VN_DEC_REF(child);
        }
        
        return res;
    }

    return -(ENOTSUP);
}

int
vops_utimes(struct proc *proc, const char *path, struct timeval times[2])
{
    struct vnode *root = proc->root;
    struct vnode *cwd = proc->cwd;

    struct vnode *child = NULL;

    if (vn_open(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }
    
    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    struct vops *ops = child->ops;

    if (ops && ops->utimes) {
        return ops->utimes(child, times);
    }

    return -(ENOTSUP);

}

int
vops_write(struct file *file, const char *buf, size_t nbyte)
{
    if (file->flags == O_RDONLY) {
        return -(EPERM);
    }

    struct vnode *node = file->node;
    struct vops *ops = node->ops;

    if (ops->write) {
        int written = ops->write(node, buf, nbyte, file->position);

        file->position += written;

        return written;
    }

    return -(EPERM);
}
