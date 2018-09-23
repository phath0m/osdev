#ifndef SYS_VFS_H
#define SYS_VFS_H

#include <rtl/types.h>
#include <sys/device.h>
#include <sys/limits.h>

struct dirent;
struct file;
struct filesystem;
struct fs_mount;
struct fs_ops;

struct vfs_node;

typedef int (*file_close_t)(struct file *file);

typedef int (*fs_close_t)(struct vfs_node *node);
typedef int (*fs_readdirent_t)(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
typedef int (*fs_open_t)(struct fs_mount*fs, struct vfs_node *node, const char *path);
typedef int (*fs_read_t)(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
typedef int (*fs_write_t)(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct dirent {
    ino_t   inode;
    char    name[PATH_MAX];
};

struct file {
    struct vfs_node *   node;
    uint64_t            position;
};

struct filesystem {
    struct device *             device;
    struct fs_ops *             methods;
};

struct fs_mount {
    struct device *     device;
    struct filesystem * filesystem;
};

struct fs_ops {
    fs_close_t      close;
    fs_open_t       open;
    fs_read_t       read;
    fs_readdirent_t readdirent;
    fs_write_t      write;
};

struct vfs_node {
    struct device *     device;
    struct filesystem * filesystem;
    ino_t               inode;
    gid_t               group;
    uid_t               owner;
    void *              state;
};


void register_filesystem(const char *name, struct fs_ops *ops);

#endif
