/*
 * vfs.c
 *
 * This file is responsible for exposing the virtual filesystem to the rest of
 * kernel space
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
#include <ds/list.h>
#include <sys/cdev.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/pipe.h>
#include <sys/string.h>
#include <sys/unistd.h>
#include <sys/vnode.h>

// delete me
#include <sys/systm.h>


/*
 * First, implement methods for struct file * interface to map them to their
 * corresponding vnode counterparts
 */
static int
vfop_chmod(struct file *fp, mode_t mode)
{
    struct vnode *vn = fp->state;

    if (vn) {
        return vop_fchmod(vn, mode);
    }

    return -(ENOTSUP);

}

static int
vfop_chown(struct file *fp, uid_t owner, uid_t group)
{
    struct vnode *vn = fp->state;

    if (vn) {
        return vop_fchown(vn, owner, group);
    }

    return -(ENOTSUP);
}

static int
vfop_close(struct file *fp)
{
    struct vnode *vn = fp->state;

    if (vn) {
        VN_DEC_REF(vn);
    }

    return 0;
}

static int
vfop_destroy(struct file *fp)
{
    return 0;
}

static int
vfop_duplicate(struct file *fp)
{
    struct vnode *vn = fp->state;

    if (vn) {
        VN_INC_REF(vn);
    }

    return 0;
}

static int
vfop_getdev(struct file *fp, struct cdev **result)
{
    struct vnode *vn = fp->state;

    if (vn && vn->device) {
        *result = vn->device;
        return 0;
    }

    return -(ENOTSUP);
}

static int
vfop_getvn(struct file *fp, struct vnode **result)
{
    struct vnode *vn = fp->state;

    if (vn) {
        *result = vn;
        return 0;
    }

    return -(ENOTSUP);
}

static int
vfop_ioctl(struct file *fp, uint64_t request, void *arg)
{
    return -(ENOTSUP);
}

static int
vfop_readdirent(struct file *fp, struct dirent *dirent, uint64_t entry)
{
    struct vnode *vn = fp->state;

    if (vn) {
        return vop_readdirent(vn, dirent, entry);
    }

    return -(ENOTSUP);
}

static int
vfop_read(struct file *fp, void *buf, size_t nbyte)
{
    struct vnode *vn = fp->state;

    if (vn) {
        return vop_read(vn, buf, nbyte, FILE_POSITION(fp));
    }

    return -(ENOTSUP);
}

static int
vfop_seek(struct file *fp, off_t *pos, off_t off, int whence)
{
    struct vnode *vn = fp->state;

    struct stat stat;

    if (vop_stat(vn, &stat) != 0) {
        return -(ESPIPE);
    }

    off_t new_pos;

    switch (whence) {
        case SEEK_CUR:
            new_pos = *pos + off;
            break;
        case SEEK_END:
            new_pos = stat.st_size - off;
            break;
        case SEEK_SET:
            new_pos = off;
            break;
    }

    if (new_pos < 0) {
        return -(ESPIPE);
    }

    *pos = new_pos;

    return new_pos;
}

static int
vfop_stat(struct file *fp, struct stat *stat)
{
    struct vnode *vn = fp->state;

    if (vn) {
        return vop_stat(vn, stat);
    }
    
    return -(ENOTSUP);
}

static int
vfop_truncate(struct file *fp, off_t length)
{
    struct vnode *vn = fp->state;

    if (vn) {
        return vop_ftruncate(vn, length);
    }

    return -(ENOTSUP);
}

static int
vfop_write(struct file *fp, const void *buf, size_t nbyte)
{
    struct vnode *vn = fp->state;

    if (vn) {
        vop_write(vn, buf, nbyte, FILE_POSITION(fp));
    }

    return -(ENOTSUP);
}

/* file operation struct for file interface */
struct fops vfs_ops = {
    .chmod      = vfop_chmod,
    .chown      = vfop_chown,
    .close      = vfop_close,
    .destroy    = vfop_destroy,
    .duplicate  = vfop_duplicate,
    .getdev     = vfop_getdev,
    .getvn      = vfop_getvn,
    .ioctl      = vfop_ioctl,    
    .read       = vfop_read,
    .readdirent = vfop_readdirent,
    .seek       = vfop_seek,
    .stat       = vfop_stat,
    .truncate   = vfop_truncate,
    .write      = vfop_write
};

/* helper function to split directory and filename from a full path string */
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

int
vfs_access(struct proc *proc, const char *path, int mode)
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
vfs_chmod(struct proc *proc, const char *path, mode_t mode)
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
vfs_chown(struct proc *proc, const char *path, uid_t owner, gid_t group)
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
vfs_creat(struct proc *proc, struct file **result, const char *path, mode_t mode)
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

    struct file *file = file_new(&vfs_ops, child);

    VN_INC_REF(child);

    file->flags = O_WRONLY | O_CREAT;

    *result = file;

    return 0;
}

int
vfs_mkdir(struct proc *proc, const char *path, mode_t mode)
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
vfs_mknod(struct proc *proc, const char *path, mode_t mode, dev_t dev)
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
vfs_rmdir(struct proc *proc, const char *path)
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
vfs_open(struct proc *proc, struct file **result, const char *path, int flags)
{   
    return vfs_open_r(proc, result, path, flags);
}

int
vfs_open_r(struct proc *proc, struct file **result, const char *path, int flags)
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

        struct file *file = NULL;

        if ((child->mode & S_IFIFO)) {
            file = fifo_to_file(child, flags);
        }

        if ((child->mode & S_IFCHR)) {
            file = cdev_to_file(child, child->devno);
        }

        if (!file) {
            file = file_new(&vfs_ops, child);
        }

        file->flags = flags;

        VN_INC_REF(child);

        *result = file;

        return 0;
    }

    if ((flags & O_CREAT)) {
        return vfs_creat(proc, result, path, 0700);
    }
        
    return -(ENOENT);
}

int
vfs_truncate(struct proc *proc, const char *path, off_t length)
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
vfs_unlink(struct proc *proc, const char *path)
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
vfs_utimes(struct proc *proc, const char *path, struct timeval times[2])
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
