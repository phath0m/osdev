#ifndef SYS_VFS_H
#define SYS_VFS_H

#include <ds/dict.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/limits.h>

#define DT_REG      0x01
/* There are no block devices */
#define DT_CHR      0x02
#define DT_DIR      0x04

#define F_DUPFD     0x00
#define F_GETFD     0x01
#define F_SETFD     0x02

#define MS_RDONLY   0x01
#define MS_NOEXEC   0x08

#define O_RDONLY    0x00
#define O_WRONLY    0x01
#define O_RDWR      0x02

#define O_CLOEXEC   0x80000

#define SEEK_SET    0x00
#define SEEK_CUR    0x01
#define SEEK_END    0x02

#define	S_IRWXU     0000700
#define	S_IRUSR     0000400
#define	S_IWUSR     0000200
#define	S_IXUSR     0000100


#define	S_IRWXG     0000070
#define	S_IRGRP     0000040
#define	S_IWGRP     0000020
#define	S_IXGRP     0000010
#define	S_IRWXO     0000007
#define	S_IROTH     0000004
#define	S_IWOTH     0000002
#define	S_IXOTH     0000001

#define INC_NODE_REF(p) __sync_fetch_and_add(&(p)->refs, 1)
#define DEC_NODE_REF(p) if (__sync_fetch_and_sub(&(p)->refs, 1) == 1) vfs_node_destroy(p);

struct dirent;
struct file;
struct filesystem;
struct fs_mount;
struct fs_ops;
struct stat;
struct vfs_node;

typedef int (*file_close_t)(struct file *file);

typedef int (*fs_close_t)(struct vfs_node *node);
typedef int (*fs_lookup_t)(struct vfs_node *parent, struct vfs_node **result, const char *name);
typedef int (*fs_mount_t)(struct device *dev, struct vfs_node **root);
typedef int (*fs_readdirent_t)(struct vfs_node *node, struct dirent *dirent, uint64_t entry);
typedef int (*fs_read_t)(struct vfs_node *node, void *buf, size_t nbyte, uint64_t pos);
typedef int (*fs_seek_t)(struct vfs_node *node, uint64_t *pos, off_t off, int whence);
typedef int (*fs_stat_t)(struct vfs_node *node, struct stat *stat);
typedef int (*fs_write_t)(struct vfs_node *node, const void *buf, size_t nbyte, uint64_t pos);

struct dirent {
    ino_t   inode;
    uint8_t type;
    char    name[PATH_MAX];
};

struct file {
    struct vfs_node *   node;
    char                name[PATH_MAX];
    int                 flags;
    uint64_t            position;
};

struct file_ops {
    fs_close_t      close;
    fs_lookup_t     lookup;
    fs_read_t       read;
    fs_readdirent_t readdirent;
    fs_seek_t       seek;
    fs_stat_t       stat;
    fs_write_t      write;
};

struct filesystem {
    char *          name;
    struct fs_ops * ops;
};

struct fs_ops {
    fs_close_t      close;
    fs_lookup_t     lookup;
    fs_mount_t      mount;
    fs_read_t       read;
    fs_readdirent_t readdirent;
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
 * vfs_close
 * Releases a file struct and frees any associated resources
 * @param file  Pointer to the open file struct
 */
int vfs_close(struct file *file);

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
 * vfs_write
 * Writes n bytes to the supplied file
 * @param file      pointer to an open file
 * @param buf       pointer to the data to write
 * @param nbyte     how many bytes to write
 */
int vfs_write(struct file *file, const char *buf, size_t nbyte);

#endif
