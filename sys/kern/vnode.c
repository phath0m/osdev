#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/pool.h>
#include <sys/string.h>
#include <sys/vnode.h>

int vfs_node_count = 0;

struct pool vn_pool;

static void
free_vfs_node_child(void *p)
{
    VN_DEC_REF((struct vnode*)p);
}

void
vn_destroy(struct vnode *node)
{
    struct vops *ops = node->ops;

    if (ops && ops->destroy) {
        ops->destroy(node);
    }    

    dict_clear_f(&node->children, free_vfs_node_child);
    
    vfs_node_count--;

    pool_put(&vn_pool, node);
}

struct vnode *
vn_new(struct vnode *parent, struct cdev *dev, struct vops *ops)
{
    struct vnode *node = pool_get(&vn_pool);

    if (parent) {
        VN_INC_REF(parent);
    }

    node->parent = parent;   
    node->device = dev;
    node->ops = ops;

    vfs_node_count++;
    return node;
}

int
vn_open(struct vnode *root, struct vnode *cwd, struct vnode **result, const char *path)
{
    if (*path == 0) {
        return -(ENOENT);
    }

    char dir[PATH_MAX];
    char *nextdir = NULL;

    struct vnode *parent = root;
    struct vnode *child = NULL;

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

        int res = vn_lookup(parent, &child, dir);

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

int
vn_lookup(struct vnode *parent, struct vnode **result, const char *name)
{
    struct vnode *node = NULL;

    int res = ENOENT;

    if (!strcmp(".", name)) {
        node = parent;
        res = 0;
    } else if (dict_get(&parent->children, name, (void**)&node)) {
        res = 0;
    } else if (parent->ops->lookup(parent, &node, name) == 0) {
        dict_set(&parent->children, name, node);

        node->mount_flags = parent->mount_flags;

        VN_INC_REF(node);

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
vop_fchmod(struct vnode *node, mode_t mode)
{       
    struct vops *ops = node->ops;
    
    if (ops && ops->chmod) {
        return ops->chmod(node, mode);
    }

    return -(ENOTSUP);
}

int
vop_fchown(struct vnode *node, uid_t owner, gid_t group)
{
    struct vops *ops = node->ops;

    if (ops && ops->chown) {
        return ops->chown(node, owner, group);
    }

    return -(ENOTSUP);
}

int
vop_ftruncate(struct vnode *node, off_t length)
{
    struct vops *ops = node->ops;

    if (ops && ops->truncate) {
        return ops->truncate(node, length);
    }

    return -(ENOTSUP);
}

int
vop_read(struct vnode *node, char *buf, size_t nbyte, off_t offset)
{
    struct vops *ops = node->ops;

    if (ops && ops->read) {
        return ops->read(node, buf, nbyte, offset);
    }

    return -(EPERM);
}

int
vop_readdirent(struct vnode *node, struct dirent *dirent, int dirno)
{
    struct vops *ops = node->ops;

    if (ops && ops->readdirent) {
        return ops->readdirent(node, dirent, dirno);
    }

    return -(EPERM);
}

int
vop_stat(struct vnode *node, struct stat *stat)
{
    struct vops *ops = node->ops;

    if (ops->stat) {
        return ops->stat(node, stat);
    }

    return -(ENOTSUP);
}

int
vop_write(struct vnode *node, const char *buf, size_t nbyte, off_t offset)
{
    struct vops *ops = node->ops;

    if (ops->write) {
        int written = ops->write(node, buf, nbyte, offset);

        return written;
    }

    return -(EPERM);
}

__attribute__((constructor))
static void
vnode_init()
{
    pool_init(&vn_pool, sizeof(struct vnode));
}
