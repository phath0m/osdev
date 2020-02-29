#ifndef _SYS_FILE_H
#define _SYS_FILE_H

#include <sys/device.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vnode.h>

struct file;

typedef int (*f_chmod_t)(struct file *fp, mode_t mode);
typedef int (*f_chown_t)(struct file *fp, uid_t owner, uid_t group);
typedef int (*f_close_t)(struct file *fp);
typedef int (*f_destroy_t)(struct file *fp);
typedef int (*f_duplicate_t)(struct file *fp);
typedef int (*f_getdev_t)(struct file *fp, struct cdev **result);
typedef int (*f_getvn_t)(struct file *fp, struct vnode **result);
typedef int (*f_ioctl_t)(struct file *fp, uint64_t request, void *arg);
typedef int (*f_mmap_t)(struct file *fp, uintptr_t addr, size_t size, int prot, off_t offset);
typedef int (*f_readdirent_t)(struct file *fp, struct dirent *dirent, uint64_t entry);
typedef int (*f_read_t)(struct file *fp, void *buf, size_t nbyte);
typedef int (*f_seek_t)(struct file *fp, off_t *pos, off_t off, int whence);
typedef int (*f_stat_t)(struct file *fp, struct stat *stat);
typedef int (*f_truncate_t)(struct file *fp, off_t length);
typedef int (*f_write_t)(struct file *fp, const void *buf, size_t nbyte);


/* file operations */
struct fops {
    f_chmod_t       chmod;
    f_chown_t       chown;
    f_close_t       close;
    f_destroy_t     destroy;
    f_duplicate_t   duplicate;
    f_getdev_t      getdev;
    f_getvn_t       getvn;
    f_ioctl_t       ioctl;
    f_mmap_t        mmap;
    f_readdirent_t  readdirent;
    f_read_t        read;
    f_seek_t        seek;
    f_stat_t        stat;
    f_truncate_t    truncate;
    f_write_t       write;
};

/* 
 * represents an open file. Usually, this corresponds to a file descriptor in
 * user space. This can be thought of being the equivelent of C's FILE struct
 */
struct file {
    struct fops *       ops;
    int                 flags;
    int                 refs;
    void *              state;
    off_t               position;
};

#define FILE_POSITION(fp) ((fp)->position)

struct file *file_new(struct fops *fops, void *state);
struct file *vfs_duplicate_file(struct file *file);

int fop_close(struct file *file);
int fop_fchmod(struct file *file, mode_t mode);
int fop_fchown(struct file *file, uid_t owner, gid_t group);
int fop_ftruncate(struct file *fp, off_t length);
int fop_close(struct file *file);
int fop_creat(struct proc *proc, struct file **result, const char *path, mode_t mode);
int fop_getdev(struct file *fp, struct cdev **result);
int fop_getvn(struct file *fp, struct vnode **result);
int fop_ioctl(struct file *fp, uint64_t request, void *arg);
int fop_mmap(struct file *file, uintptr_t addr, size_t size, int prot, off_t offset);
int fop_read(struct file *file, char *buf, size_t nbyte);
int fop_readdirent(struct file *file, struct dirent *dirent);
int fop_seek(struct file *file, off_t off, int whence);
int fop_stat(struct file *file, struct stat *stat);
int fop_write(struct file *file, const char *buf, size_t nbyte);


#endif
