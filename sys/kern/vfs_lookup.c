#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/string.h>
#include <sys/vfs.h>
// remove me
#include <sys/systm.h>

int vfs_node_count = 0;

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
vfs_node_new(struct vfs_node *parent, struct device *dev, struct file_ops *ops)
{
    struct vfs_node *node = (struct vfs_node*)calloc(0, sizeof(struct vfs_node));
    
    if (parent) {
        INC_NODE_REF(parent);
    }

    node->parent = parent;   
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
