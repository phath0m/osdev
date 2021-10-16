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


int vfs_file_count = 0; /* internal counter of file objects */
struct list fs_list;    /* registered filesystem types */
struct pool file_pool;  /* file objects */ 

static struct filesystem *
getfsbyname(const char *name)
{
    list_iter_t iter;

    struct filesystem *fs;
    struct filesystem *res;

    list_get_iter(&fs_list, &iter);

    res = NULL;

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
file_new(struct fops *ops, void *state)
{
    struct file *file;
    
    file = pool_get(&file_pool);

    file->ops = ops;
    file->state = state;
    file->refs = 1;
    
    vfs_file_count++;

    return file;
}

int
file_close(struct file *file)
{
    struct fops *ops;
    
    if ((--file->refs) > 0) {
        return 0;
    }

    ops = file->ops;
    
    if (ops->close) {
        ops->close(file);
    }

    vfs_file_count--;
    
    pool_put(&file_pool, file);

    return 0; 
}

struct file *
file_duplicate(struct file *file)
{
    struct file *new_file;
    struct fops *ops;

    if (!file) {
        return NULL;
    }

    new_file = pool_get(&file_pool);

    memcpy(new_file, file, sizeof(struct file));

    new_file->refs = 1;

    vfs_file_count++;

    ops = new_file->ops;

    if (ops && ops->duplicate) {
        ops->duplicate(new_file);
    }

    return new_file;
}


/* filesystem routines */
int
fs_open(struct file *dev_fp, struct vnode **vn_res, const char *fsname, int flags)
{
    int res;
    struct filesystem *fs;
    struct vnode *vn;

    fs = getfsbyname(fsname);

    if (fs) {
        vn = NULL;

        res = fs->ops->mount(NULL, dev_fp, &vn);

        if (vn) {
            vn->mount_flags = flags;
            *vn_res = vn;
        }

        return res;
    }

    return -1;
}

void
fs_register(char *name, struct fs_ops *ops)
{
    struct filesystem *fs;
    
    fs = malloc(sizeof(struct filesystem));

    fs->name = name;
    fs->ops = ops;

    list_append(&fs_list, fs);
}

int
fs_mount(struct vnode *root, const char *dev, const char *fsname, const char *path, int flags)
{
    int res;
    struct vnode *mount;
    struct file *dev_fp;
    struct file *mount_fp;
    struct vnode *mount_point;

    mount_fp = NULL;
    dev_fp = NULL;

    if (dev && vfs_open(current_proc, &dev_fp, dev, O_RDONLY) != 0) {
        return -(ENOENT);
    }

    res = fs_open(dev_fp, &mount, fsname, flags);

    if (res != 0) goto cleanup;

    if (vfs_open(current_proc, &mount_fp, path, O_RDONLY) == 0) {
        if (FOP_GETVN(mount_fp, &mount_point) == 0) { 
            mount_point->ismount = true;
            mount_point->mount = mount;
        
            VN_INC_REF(mount);
        }
    } else {
        res = -1;
    }

cleanup:

    if (dev_fp) {
        file_close(dev_fp);
    }

    return res;
}

bool
fs_probe(struct cdev *dev, const char *fsname, int uuid_length, const uint8_t *uuid)
{
    struct filesystem *fs;
    
    fs = getfsbyname(fsname);

    if (!fs) {
        return false;

    }

    return fs->ops->probe(dev, uuid_length, uuid);
}

/*
 * Methods for binding struct file * instances to filesystem
 */
static int
vfop_chmod(struct file *fp, mode_t mode)
{
    struct vnode *vn;
    
    vn = fp->state;

    if (vn) {
        return VOP_FCHMOD(vn, mode);
    }

    return -(ENOTSUP);

}

static int
vfop_chown(struct file *fp, uid_t owner, uid_t group)
{
    struct vnode *vn;
    
    vn = fp->state;

    if (vn) {
        return VOP_FCHOWN(vn, owner, group);
    }

    return -(ENOTSUP);
}

static int
vfop_close(struct file *fp)
{
    struct vnode *vn;
    
    vn = fp->state;

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
    struct vnode *vn;
    
    vn = fp->state;

    if (vn) {
        VN_INC_REF(vn);
    }

    return 0;
}

static int
vfop_getdev(struct file *fp, struct cdev **result)
{
    struct vnode *vn;
    
    vn = fp->state;

    if (vn && vn->device) {
        *result = vn->device;
        return 0;
    }

    return -(ENOTSUP);
}

static int
vfop_getvn(struct file *fp, struct vnode **result)
{
    struct vnode *vn;
    
    vn = fp->state;

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
    struct vnode *vn;
    
    vn = fp->state;

    if (vn) {
        return VOP_READDIRENT(vn, dirent, entry);
    }

    return -(ENOTSUP);
}

static int
vfop_read(struct file *fp, void *buf, size_t nbyte)
{
    struct vnode *vn;
    
    vn = fp->state;

    if (vn) {
        return VOP_READ(vn, buf, nbyte, FILE_POSITION(fp));
    }

    return -(ENOTSUP);
}

static int
vfop_seek(struct file *fp, off_t *pos, off_t off, int whence)
{
    off_t new_pos;
    struct stat stat;
    struct vnode *vn;

    vn = fp->state;

    if (VOP_STAT(vn, &stat) != 0) {
        return -(ESPIPE);
    }

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
    struct vnode *vn;
    
    vn = fp->state;

    if (vn) {
        return VOP_STAT(vn, stat);
    }
    
    return -(ENOTSUP);
}

static int
vfop_truncate(struct file *fp, off_t length)
{
    struct vnode *vn;
    
    vn = fp->state;

    if (vn) {
        return VOP_FTRUNCATE(vn, length);
    }

    return -(ENOTSUP);
}

static int
vfop_write(struct file *fp, const void *buf, size_t nbyte)
{
    struct vnode *vn;
    
    vn = fp->state;

    if (vn) {
        return VOP_WRITE(vn, buf, nbyte, FILE_POSITION(fp));
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
    int i;
    int last_slash = -1;

    last_slash = -1;

    for (i = 0; i < PATH_MAX && path[i]; i++) {
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

/* actual VFS routines */

int
vfs_access(struct proc *proc, const char *path, int mode)
{
    bool will_read;
    bool will_write;

    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;

    root = proc->root;
    cwd = proc->cwd;

    will_write = mode & W_OK;
    will_read = mode & R_OK;

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
    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;
    struct vops *ops;

    root = proc->root;
    cwd = proc->cwd;
    child = NULL;

    if (vn_open(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }

    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    ops = child->ops;

    if (ops && ops->chmod) {
        return ops->chmod(child, mode);
    }

    return -(ENOTSUP);
}

int
vfs_chown(struct proc *proc, const char *path, uid_t owner, gid_t group)
{
    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;
    struct vops *ops;

    root = proc->root;
    cwd = proc->cwd;
    child = NULL;

    if (vn_open(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }

    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    ops = child->ops;

    if (ops && ops->chown) {
        return ops->chown(child, owner, group);
    }

    return -(ENOTSUP);
}

int
vfs_creat(struct proc *proc, struct file **result, const char *path, mode_t mode)
{
    int res;
    char *filename;
    char *parent_path;

    struct file *file;
    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;
    struct vops *ops;

    char path_buf[PATH_MAX+1];

    struct vnode *parent;

    root = proc->root;
    cwd = proc->cwd;

    strncpy(path_buf, path, PATH_MAX);

    if (!split_path(path_buf, &parent_path, &filename)) {
        parent = cwd;
    } else if (vn_open(root, cwd, &parent, parent_path) != 0) {
        return -(ENOENT);
    }

    if ((parent->mount_flags & MS_RDONLY)) {
        return -(EROFS);
    }

    ops = parent->ops;

    if (!ops || !ops->creat) {
        return -(ENOTSUP);
    }

    if (!can_write(parent, &proc->creds)) {
        return -(EACCES);
    }

    res = ops->creat(parent, &child, filename, mode);

    if (res != 0) {
        return res;
    }

    file = file_new(&vfs_ops, child);

    VN_INC_REF(child);

    file->flags = O_WRONLY | O_CREAT;

    *result = file;

    return 0;
}

int
vfs_mkdir(struct proc *proc, const char *path, mode_t mode)
{
 
    char *dirname;
    char *parent_path;
    struct vnode *parent;

    struct vnode *root;
    struct vnode *cwd;
    struct vops *ops;

    char path_buf[PATH_MAX+1];

    strncpy(path_buf, path, PATH_MAX);

    root = proc->root;
    cwd = proc->cwd; 

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

    ops = parent->ops;

    if (ops && ops->mkdir) {
        return ops->mkdir(parent, dirname, mode);
    }

    return -(ENOTSUP);
}

int
vfs_mknod(struct proc *proc, const char *path, mode_t mode, dev_t dev)
{
    char *parent_path;
    char *nodename;
    struct vnode *parent;

    struct vnode *root;
    struct vnode *cwd;
    struct vops *ops;

    char path_buf[PATH_MAX+1];

    root = proc->root;
    cwd = proc->cwd; 

    strncpy(path_buf, path, PATH_MAX);

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

    ops = parent->ops;

    if (ops && ops->mknod) {
        return ops->mknod(parent, nodename, mode, dev);
    }

    return -(ENOTSUP);
}

int
vfs_rmdir(struct proc *proc, const char *path)
{
    int res;
    char *dirname;
    char *parent_path;

    struct vnode *parent;
    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;
    struct vops *ops;

    char path_buf[PATH_MAX+1];

    root = proc->root;
    cwd = proc->cwd;

    strncpy(path_buf, path, PATH_MAX);

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

    ops = parent->ops;

    if (ops && ops->rmdir) {
        res = ops->rmdir(parent, dirname);
        
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
    bool will_read;
    bool will_write;

    struct file *file;
    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;

    file = NULL;
    root = proc->root;
    cwd = proc->cwd;

    if (vn_open(root, cwd, &child, path) == 0) {
        will_write = (flags & O_WRONLY);
        will_read = true; // this is a hack.

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
    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;
    struct vops *ops;

    root = proc->root;
    cwd = proc->cwd;
    child = NULL;

    if (vn_open(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }
    
    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    ops = child->ops;

    if (ops && ops->truncate) {
        return ops->truncate(child, length);
    }

    return -(ENOTSUP);
}

int
vfs_unlink(struct proc *proc, const char *path)
{
    int res;
    char *filename;
    char *parent_path;

    struct vnode *parent;
    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;
    struct vops *ops;

    char path_buf[PATH_MAX+1];

    root = proc->root;
    cwd = proc->cwd;

    strncpy(path_buf, path, PATH_MAX);

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

    ops = parent->ops;

    if (ops && ops->unlink) {
        res = ops->unlink(parent, filename);
        
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
    struct vnode *root;
    struct vnode *cwd;
    struct vnode *child;
    struct vops *ops;

    root = proc->root;
    cwd = proc->cwd;

    if (vn_open(root, cwd, &child, path) != 0) {
        return -(ENOENT);
    }
    
    if (child->mount_flags & MS_RDONLY) {
        return -(EROFS);
    }

    if (!can_write(child, &proc->creds)) {
        return -(EACCES);
    }

    ops = child->ops;

    if (ops && ops->utimes) {
        return ops->utimes(child, times);
    }

    return -(ENOTSUP);
}