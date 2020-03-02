#include <ds/list.h>
#include <sys/device.h>
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
    struct file *file = pool_get(&file_pool);

    file->ops = ops;
    file->state = state;
    file->refs = 1;
    
    vfs_file_count++;

    return file;
}

int
fop_close(struct file *file)
{
    if ((--file->refs) > 0) {
        return 0;
    }

    struct fops *ops = file->ops;
    
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
    if (!file) {
        return NULL;
    }

    struct file *new_file = pool_get(&file_pool);

    memcpy(new_file, file, sizeof(struct file));

    new_file->refs = 1;

    vfs_file_count++;

    struct fops *ops = new_file->ops;

    if (ops && ops->duplicate) {
        ops->duplicate(new_file);
    }

    return new_file;
}

int
fop_fchmod(struct file *fp, mode_t mode)
{
    struct fops *ops = fp->ops;

    if (ops && ops->chmod) {
        return ops->chmod(fp, mode);
    }

    return -(ENOTSUP);
}

int
fop_fchown(struct file *fp, uid_t owner, gid_t group)
{
    struct fops *ops = fp->ops;

    if (ops && ops->chown) {
        return ops->chown(fp, owner, group);
    }

    return -(ENOTSUP);
}

int
fop_ftruncate(struct file *fp, off_t length)
{
    struct fops *ops = fp->ops;

    if (ops && ops->truncate) {
        return ops->truncate(fp, length);
    }

    return -(ENOTSUP);
}

int
fop_getdev(struct file *fp, struct cdev **result)
{
    struct fops *ops = fp->ops;

    if (ops && ops->getdev) {
        return ops->getdev(fp, result);
    }

    return -(ENOTSUP);
}

int
fop_getvn(struct file *fp, struct vnode **result)
{
    struct fops *ops = fp->ops;
    
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
    struct fops *ops = fp->ops;

    if (ops->mmap) {
        return ops->mmap(fp, addr, size, prot, offset);
    }

    return -(ENOTSUP);
}

int
fop_read(struct file *fp, char *buf, size_t nbyte)
{
    if (fp->flags == O_WRONLY) {
        return -(EPERM);
    }

    struct fops *ops = fp->ops;

    if (ops->read) {
        int read = ops->read(fp, buf, nbyte);
    
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
    struct fops *ops = fp->ops;

    if (ops->readdirent) {
        int res = ops->readdirent(fp, dirent, fp->position);

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
    struct fops *ops = fp->ops;

    if (ops->seek) {
        return ops->seek(fp, &fp->position, off, whence);
    }

    return -(ESPIPE);
}

int
fop_stat(struct file *fp, struct stat *stat)
{
    struct fops *ops = fp->ops;

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
    if (fp->flags == O_RDONLY) {
        return -(EPERM);
    }

    struct fops *ops = fp->ops;

    if (ops->write) {
        int written = ops->write(fp, buf, nbyte);

        if (written > 0) {
            fp->position += written;
        }

        return written;
    }

    return -(EPERM);
}

__attribute__((constructor))
static void
file_init()
{
    pool_init(&file_pool, sizeof(struct file));
}
