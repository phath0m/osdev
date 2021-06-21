/*
 * file.c
 *
 * Implements methods for the struct file * interface (Akin to C's FILE*)
 *
 * In the kernel, anything that compatible with the read()/write() system
 * calls is a file implements its read()/write() functionality ontop of a
 * struct file instance.
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
#include <sys/pool.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/unistd.h>
#include <sys/vnode.h>

// delete me
#include <sys/systm.h>

int vfs_file_count = 0;

struct pool file_pool;

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
fop_close(struct file *file)
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
vfs_duplicate_file(struct file *file)
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

int
fop_fchmod(struct file *fp, mode_t mode)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops && ops->chmod) {
        return ops->chmod(fp, mode);
    }

    return -(ENOTSUP);
}

int
fop_fchown(struct file *fp, uid_t owner, gid_t group)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops && ops->chown) {
        return ops->chown(fp, owner, group);
    }

    return -(ENOTSUP);
}

int
fop_ftruncate(struct file *fp, off_t length)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops && ops->truncate) {
        return ops->truncate(fp, length);
    }

    return -(ENOTSUP);
}

int
fop_getdev(struct file *fp, struct cdev **result)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops && ops->getdev) {
        return ops->getdev(fp, result);
    }

    return -(ENOTSUP);
}

int
fop_getvn(struct file *fp, struct vnode **result)
{
    struct fops *ops;
    
    ops = fp->ops;
    
    if (ops && ops->getvn) {
        return ops->getvn(fp, result);
    }
    
    return -(ENOTSUP);
}

int
fop_ioctl(struct file *fp, uint64_t request, void *arg)
{
    struct fops *ops = fp->ops;

    if (ops->ioctl) {
        return ops->ioctl(fp, request, arg);
    }

    return -(ENOTSUP);
}

int
fop_mmap(struct file *fp, uintptr_t addr, size_t size, int prot, off_t offset)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops->mmap) {
        return ops->mmap(fp, addr, size, prot, offset);
    }

    return -(ENOTSUP);
}

int
fop_read(struct file *fp, char *buf, size_t nbyte)
{
    int read;
    struct fops *ops;

    if (fp->flags == O_WRONLY) {
        return -(EPERM);
    }

    ops = fp->ops;

    if (ops->read) {
        read = ops->read(fp, buf, nbyte);
    
        if (read > 0) {
            fp->position += read;
        }

        return read;
    }

    return -(EPERM);
}

int
fop_readdirent(struct file *fp, struct dirent *dirent)
{
    int res;
    struct fops *ops;
    
    ops = fp->ops;

    if (ops->readdirent) {
        res = ops->readdirent(fp, dirent, fp->position);

        if (res == 0) {
            fp->position++;
        }

        return res;
    }

    return -(EPERM);
}

int
fop_seek(struct file *fp, off_t off, int whence)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops->seek) {
        return ops->seek(fp, &fp->position, off, whence);
    }

    return -(ESPIPE);
}

int
fop_stat(struct file *fp, struct stat *stat)
{
    struct fops *ops;
    
    ops = fp->ops;

    if (ops->stat) {
        return ops->stat(fp, stat);
    }

    return -(ENOTSUP);
}

off_t
fop_tell(struct file *fp)
{
    return fp->position;
}

int
fop_write(struct file *fp, const char *buf, size_t nbyte)
{
    ssize_t written;
    struct fops *ops;

    if (fp->flags == O_RDONLY) {
        return -(EPERM);
    }

    ops = fp->ops;

    if (ops->write) {
        written = ops->write(fp, buf, nbyte);

        if (written > 0) {
            fp->position += (off_t)written;
        }

        return written;
    }

    return -(EPERM);
}
