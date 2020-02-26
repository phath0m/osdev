#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/string.h>
#include <sys/vnode.h>

struct list fs_list;

static struct filesystem *
getfsbyname(const char *name)
{
    list_iter_t iter;

    list_get_iter(&fs_list, &iter);

    struct filesystem *fs;
    struct filesystem *res = NULL;

    while (iter_move_next(&iter, (void**)&fs)) {
        if (strcmp(name, fs->name) == 0) {
            res = fs;
            break;
        }
    }
    
    iter_close(&iter);

    return res;
}

int
fs_open(struct cdev *dev, struct vnode **root, const char *fsname, int flags)
{
    struct filesystem *fs = getfsbyname(fsname);

    if (fs) {
        struct vnode *node = NULL;

        int result = fs->ops->mount(NULL, dev, &node);

        if (node) {
            node->mount_flags = flags;
            *root = node;
        }

        return result;
    }

    return -1;
}

void
fs_register(char *name, struct fs_ops *ops)
{
    struct filesystem *fs = (struct filesystem*)malloc(sizeof(struct filesystem));

    fs->name = name;
    fs->ops = ops;

    list_append(&fs_list, fs);
}

int
fs_mount(struct vnode *root, struct cdev *dev, const char *fsname, const char *path, int flags)
{
    struct vnode *mount;
    struct file *file;

    if (fs_open(dev, &mount, fsname, flags) == 0) {
        if (vops_open(current_proc, &file, path, O_RDONLY) == 0) {
            struct vnode *mount_point = file->node;
            
            mount_point->ismount = true;
            mount_point->mount = mount;

            VN_INC_REF(mount);

            return 0;
        }
    }

    return -1;
}
