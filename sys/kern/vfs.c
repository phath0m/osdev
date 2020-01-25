#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/string.h>
#include <sys/vfs.h>

int vfs_node_count = 0;

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
fops_openfs(struct device *dev, struct vfs_node **root, const char *fsname, int flags)
{
    struct filesystem *fs = getfsbyname(fsname);

    if (fs) {
        struct vfs_node *node = NULL;

        int result = fs->ops->mount(dev, &node);

        if (node) {
            node->mount_flags = flags;
            *root = node;
        }

        return result;
    }

    return -1;
}

struct vfs_mount *
vfs_mount_new(struct device *dev, struct filesystem *fs, uint64_t flags)
{
    struct vfs_mount *mount = (struct vfs_mount*)calloc(0, sizeof(struct vfs_mount));

    mount->device = dev;
    mount->filesystem = fs;
    mount->flags = flags;

    return mount;
}

static void
free_vfs_node_child(void *p)
{
    DEC_NODE_REF((struct vfs_node*)p);
}

void
vfs_node_destroy(struct vfs_node *node)
{
    struct file_ops *ops = node->ops;

    if (ops && ops->destroy) {
        ops->destroy(node);
    }    

    /*
     * Note: this should actually attempt to free each item in children
     * That being said, this will not get called as the reference counting
     * is sort of broken atm
     */
    //dict_clear(&node->children);
    dict_clear_f(&node->children, free_vfs_node_child);
    
    vfs_node_count--;

    free(node);
}

struct vfs_node *
vfs_node_new(struct device *dev, struct file_ops *ops)
{
    struct vfs_node *node = (struct vfs_node*)calloc(0, sizeof(struct vfs_node));
    
    node->device = dev;
    node->ops = ops;
    vfs_node_count++;
    return node;
}

int
vfs_get_node(struct vfs_node *root, struct vfs_node *cwd, struct vfs_node **result, const char *path)
{
    if (*path == 0) {
        return -(ENOENT);
    }

    char dir[PATH_MAX];
    char *nextdir = NULL;

    struct vfs_node *parent = root;
    struct vfs_node *child = NULL;

    if (cwd && *path != '/') {
        parent = cwd;
    } else if (*path == '/') {
        path++;

        if (*path == 0) {
            *result = root;
            return 0;
        }
    }

    do {
        nextdir = strrchr(path, '/');

        if (nextdir) {
            strncpy(dir, path, (nextdir - path) + 1);
            path = nextdir + 1;
        } else {
            strcpy(dir, path);
        }

        int res = vfs_lookup(parent, &child, dir);

        if (res != 0) {
            return res;
        }

        parent = child;

    } while (nextdir && *path);

    if (child) {
        *result = child;

        return 0;
    }

    return -(ENOENT);
}

void
register_filesystem(char *name, struct fs_ops *ops)
{
    struct filesystem *fs = (struct filesystem*)malloc(sizeof(struct filesystem));

    fs->name = name;
    fs->ops = ops;

    list_append(&fs_list, fs);
}

int
vfs_lookup(struct vfs_node *parent, struct vfs_node **result, const char *name)
{
    struct vfs_node *node = NULL;

    int res = ENOENT;

    if (!strcmp(".", name)) {
        node = parent;
        res = 0;
    } else if (dict_get(&parent->children, name, (void**)&node)) {
        res = 0;
    } else if (parent->ops->lookup(parent, &node, name) == 0) {
        dict_set(&parent->children, name, node);
        
        node->mount_flags = parent->mount_flags;

        INC_NODE_REF(node);

        res = 0;
    }

    if (node && node->ismount) {
        *result = node->mount;
    } else if (node) {
        *result = node;
    }

    return res;
}

int
vfs_mount(struct vfs_node *root, struct device *dev, const char *fsname, const char *path, int flags)
{
    struct vfs_node *mount;
    struct file *file;

    if (fops_openfs(dev, &mount, fsname, flags) == 0) {
        if (fops_open(root, &file, path, O_RDONLY) == 0) {
            struct vfs_node *mount_point = file->node;
            
            mount_point->ismount = true;
            mount_point->mount = mount;

            INC_NODE_REF(mount);

            return 0;
        }
    }

    return -1;
}
