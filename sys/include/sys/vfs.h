#ifndef SYS_VFS_H
#define SYS_VFS_H

#include <ds/dict.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/limits.h>

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
typedef int (*fs_close_t)(struct vfs_node *node);
typedef int (*fs_creat_t)(struct vfs_node *node, struct vfs_node **result, const char *name, mode_t mode);
typedef int (*fs_lookup_t)(struct vfs_node *parent, struct vfs_node **result, const char *name);
typedef int (*fs_mkdir_t)(struct vfs_node *parent, const char *name, mode_t mode); 
typedef int (*fs_mount_t)(struct device *dev, struct vfs_node **root);
typedef int (*fs_readdirent_t)(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
typedef int (*fs_read_t)(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
typedef int (*fs_rmdir_t)(struct vfs_node *node, const char *path);
typedef int (*fs_seek_t)(struct vfs_node *node, uint64_t *pos, off_t off, int whence);
typedef int (*fs_stat_t)(struct vfs_node *node, struct stat *stat);
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
    fs_close_t      close;
    fs_creat_t      creat;
    fs_lookup_t     lookup;
    fs_read_t       read;
    fs_readdirent_t readdirent;
    fs_rmdir_t      rmdir;
    fs_mkdir_t      mkdir;
    fs_seek_t       seek;
    fs_stat_t       stat;
    fs_unlink_t     unlink;
    fs_write_t      write;
};

struct filesystem {
    char *          name;
    struct fs_ops * ops;
};

struct fs_ops {
    fs_close_t      close;
    fs_creat_t      creat;
    fs_lookup_t     lookup;
    fs_mount_t      mount;
    fs_read_t       read;
    fs_readdirent_t readdirent;
    fs_rmdir_t      rmdir;
    fs_mkdir_t      mkdir;
    fs_unlink_t     unlink;
    fs_write_t      write;
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
    int                 mount_flags;
    bool                ismount;
    ino_t               inode;
    gid_t               group;
    uid_t               owner;
    uint64_t            size;
    void *              state;
    int                 refs;
};

struct file *file_new(struct vfs_node *node);

int vfs_close(struct file *file);

struct file *vfs_duplicate_file(struct file *file);

int vfs_openfs(struct device *dev, struct vfs_node **root, const char *fsname, int flags);

void vfs_node_destroy(struct vfs_node *node);

struct vfs_node *vfs_node_new(struct device *dev, struct file_ops *ops);

int vfs_get_node(struct vfs_node *root, struct vfs_node *cwd, struct vfs_node **result, const char *path);

int vfs_open(struct vfs_node *root, struct file **result, const char *path, int flags);

int vfs_open_r(struct vfs_node *root, struct vfs_node *cwd, struct file **result, const char *path, int flags);

void register_filesystem(char *name, struct fs_ops *ops);


/*
 * vfs_fchmod
 * @param file  The file
 * @param mode  The mode
 */
int vfs_fchmod(struct file *file, mode_t mode);

/*
 * vfs_close
 * Releases a file struct and frees any associated resources
 * @param file  Pointer to the open file struct
 */
int vfs_close(struct file *file);

/*
 * vfs_creat
 * Creates a new file
 */
int vfs_creat(struct vfs_node *root, struct vfs_node *cwd, struct file **result, const char *path, mode_t mode);

/*
 * vfs_lookup
 * Looks up a directory entry relative to the supplied VFS node
 * @param parent    The VFS node for the directory that is to be searched
 * @param result    Pointer to a vfs_node pointer that will store the result
 * @param name      Name of the child directory contained inside the parent VFS node
 */
int vfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name); 

/*
 * vfs_mount
 * Mounts a filesystem
 * @param root      VFS node pointing to the root of the filesystem
 * @param dev       Device containing filesystem
 * @param fsname    Name of the filesystem to mount
 * @param path      Path to the mount point
 * @flags           Mount flags
 */
int vfs_mount(struct vfs_node *root, struct device *dev, const char *fsname, const char *path, int flags);

/*
 * vfs_mkdir
 * Creates a new directory
 * @param root  Root filesystem
 * @param cwd   Current working directory for relative paths
 * @param path  Path to mount
 * @param mode  Mode for newly created file
 */
int vfs_mkdir(struct vfs_node *root, struct vfs_node *cwd, const char *path, mode_t mode);

/*
 * vfs_read
 * Reads n bytes from an open file
 * @param file  Pointer to an open file struct
 * @param buf   Buffer to read n bytes into
 * @param nbyte Amount of bytes to read
 */
int vfs_read(struct file *file, char *buf, size_t nbyte);

/*
 * vfs_readdirent
 * Reads a single directory entry
 * @param file      Pointer to an open file struct
 * @param dirent    Pointer to a dirent struct
 */
int vfs_readdirent(struct file *file, struct dirent *dirent);

/*
 * vfs_rmdir
 * Removes an empty directory
 * @param root  Pointer to the root filesystem
 * @param cwd   Current working directory used for relative paths
 * @param path  Path of file to delete
 */
int vfs_rmdir(struct vfs_node *root, struct vfs_node *cwd, const char *path);

/*
 * vfs_seek
 * Moves the position nbytes relative to the supplied offset
 * @param file      Pointer to an open file struct
 * @param off       Offset to move
 * @param whence    specifies origin to use relative to the supplied offset
 */
int vfs_seek(struct file *file, off_t off, int whence);

/*
 * vfs_stat
 * Obtains information about a file, writing the info into the provided stat struct
 * @param file  Pointer to an open file
 * @param stat  Pointer to a stat buf
 */
int vfs_stat(struct file *file, struct stat *stat);

/*
 * vfs_tell
 * Reports the current position in the supplied file
 * @param file  Pointer to the open file struct
 */
uint64_t vfs_tell(struct file *file);

/*
 * vfs_unlink
 * Deletes a file
 */
int vfs_unlink(struct vfs_node *root, struct vfs_node *cwd, const char *path);

/*
 * vfs_write
 * Writes n bytes to the supplied file
 * @param file      pointer to an open file
 * @param buf       pointer to the data to write
 * @param nbyte     how many bytes to write
 */
int vfs_write(struct file *file, const char *buf, size_t nbyte);

#endif
