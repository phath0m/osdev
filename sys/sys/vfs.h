#ifndef SYS_VFS_H
#define SYS_VFS_H

#include <ds/dict.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/limits.h>
#include <sys/proc.h>

#define SEEK_SET    0x00
#define SEEK_CUR    0x01
#define SEEK_END    0x02

#define INC_NODE_REF(p) __sync_fetch_and_add(&(p)->refs, 1)
#define DEC_NODE_REF(p) if (__sync_fetch_and_sub(&(p)->refs, 1) == 1) vfs_node_destroy(p);
#define INC_FILE_REF(p) __sync_fetch_and_add(&(p)->refs, 1)

struct dirent;
struct file;
struct filesystem;
struct fs_mount;
struct fs_ops;
struct stat;
struct vfs_node;

typedef int (*file_close_t)(struct file *file);

typedef int (*fs_chmod_t)(struct vfs_node *node, mode_t mode);
typedef int (*fs_chown_t)(struct vfs_node *node, uid_t owner, uid_t group);
typedef int (*fs_node_destroy_t)(struct vfs_node *node);
typedef int (*fs_close_t)(struct vfs_node *node, struct file *fp);
typedef int (*fs_creat_t)(struct vfs_node *node, struct vfs_node **result, const char *name, mode_t mode);
typedef int (*fs_duplicate_t)(struct vfs_node *node, struct file *fp);
typedef int (*fs_ioctl_t)(struct vfs_node *node, uint64_t request, void *arg);
typedef int (*fs_lookup_t)(struct vfs_node *parent, struct vfs_node **result, const char *name);
typedef int (*fs_mkdir_t)(struct vfs_node *parent, const char *name, mode_t mode); 
typedef int (*fs_mknod_t)(struct vfs_node *parent, const char *name, mode_t mode, dev_t dev);
typedef int (*fs_mount_t)(struct vfs_node *parent, struct device *dev, struct vfs_node **root);
typedef int (*fs_readdirent_t)(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
typedef int (*fs_read_t)(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
typedef int (*fs_rmdir_t)(struct vfs_node *node, const char *path);
typedef int (*fs_seek_t)(struct vfs_node *node, uint64_t *pos, off_t off, int whence);
typedef int (*fs_stat_t)(struct vfs_node *node, struct stat *stat);
typedef int (*fs_truncate_t)(struct vfs_node *node, off_t length);
typedef int (*fs_unlink_t)(struct vfs_node *parent, const char *name);
typedef int (*fs_write_t)(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct file {
    struct vfs_node *   node;
    char                name[PATH_MAX];
    int                 flags;
    int                 refs;
    uint64_t            position;
};
 
struct file_ops {
    fs_node_destroy_t       destroy;
    fs_chmod_t              chmod;
    fs_chown_t              chown;
    fs_close_t              close;
    fs_creat_t              creat;
    fs_duplicate_t          duplicate;
    fs_ioctl_t              ioctl;
    fs_lookup_t             lookup;
    fs_read_t               read;
    fs_readdirent_t         readdirent;
    fs_rmdir_t              rmdir;
    fs_mkdir_t              mkdir;
    fs_mknod_t              mknod;
    fs_seek_t               seek;
    fs_stat_t               stat;
    fs_truncate_t           truncate;
    fs_unlink_t             unlink;
    fs_write_t              write;
};

struct filesystem {
    char *          name;
    struct fs_ops * ops;
};

struct fs_ops {
    fs_node_destroy_t       destroy;
    fs_creat_t              creat;
    fs_lookup_t             lookup;
    fs_mount_t              mount;
    fs_read_t               read;
    fs_readdirent_t         readdirent;
    fs_rmdir_t              rmdir;
    fs_mkdir_t              mkdir;
    fs_unlink_t             unlink;
    fs_write_t              write;
};

struct vfs_mount {
    struct device *     device;
    struct filesystem * filesystem;
    uint64_t            flags;
};

struct vfs_node {
    struct device *     device;
    struct dict         children;
    struct file_ops *   ops;
    struct vfs_node *   mount;
    struct vfs_node *   parent;
    struct list         fifo_readers;
    int                 mount_flags;
    bool                ismount;
    ino_t               inode;
    gid_t               gid;
    uid_t               uid;
    mode_t              mode;
    uint64_t            size;
    void *              state;
    int                 refs;
};

struct vfs_node *fifo_open(struct vfs_node *parent, mode_t mode);

struct file *file_new(struct vfs_node *node);

int fops_close(struct file *file);

struct file *vfs_duplicate_file(struct file *file);

int fops_openfs(struct device *dev, struct vfs_node **root, const char *fsname, int flags);

void vfs_node_destroy(struct vfs_node *node);

struct vfs_node *vfs_node_new(struct vfs_node *parent, struct device *dev, struct file_ops *ops);

int vfs_get_node(struct vfs_node *root, struct vfs_node *cwd, struct vfs_node **result, const char *path);

int fops_open(struct proc *proc, struct file **result, const char *path, int flags);

int fops_open_r(struct proc *proc, struct file **result, const char *path, int flags);

void register_filesystem(char *name, struct fs_ops *ops);

int fops_access(struct proc *proc, const char *path, int mode);

int fops_fchmod(struct file *file, mode_t mode);

int fops_fchown(struct file *file, uid_t owner, gid_t group);

int fops_ftruncate(struct file *fp, off_t length);

int fops_chmod(struct proc *proc, const char *path, mode_t mode);

int fops_chown(struct proc *proc, const char *path, uid_t owner, gid_t group);

int fops_close(struct file *file);

int fops_creat(struct proc *proc, struct file **result, const char *path, mode_t mode);

int fops_ioctl(struct file *fp, uint64_t request, void *arg);

int vfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name); 

int vfs_mount(struct vfs_node *root, struct device *dev, const char *fsname, const char *path, int flags);

int fops_mkdir(struct proc *proc, const char *path, mode_t mode);

int fops_mknod(struct proc *proc, const char *path, mode_t mode, dev_t dev);

int fops_read(struct file *file, char *buf, size_t nbyte);

int fops_readdirent(struct file *file, struct dirent *dirent);

int fops_rmdir(struct proc *proc, const char *path);

int fops_seek(struct file *file, off_t off, int whence);

int fops_stat(struct file *file, struct stat *stat);

uint64_t fops_tell(struct file *file);

int fops_truncate(struct proc *proc, const char *path, off_t length);

int fops_unlink(struct proc *proc, const char *path);

int fops_write(struct file *file, const char *buf, size_t nbyte);

#endif
