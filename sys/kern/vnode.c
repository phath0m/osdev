/*
 * vnode.c - VFS Node
 *
 * Implements the associated methods with node mounted on the virtual
 * filesystem.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <ds/list.h>
#include <sys/cdev.h>
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
    struct vops *ops;
    
    ops = node->ops;

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
    struct vnode *node;
    
    node = pool_get(&vn_pool);

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
    int res;
    char *nextdir;
    char dir[PATH_MAX];

    struct vnode *parent;
    struct vnode *child;

    if (*path == 0) {
        return -(ENOENT);
    }

    parent = root;
    child = NULL;
    nextdir = NULL;

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

        res = vn_lookup(parent, &child, dir);

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
    int res;
    struct vnode *node;

    res = ENOENT;
    node = NULL;

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

static int
vn_resolve_name_r(struct vnode *child, struct vnode *parent, char **components, int depth)
{
    int ret;
    list_iter_t iter;

    char *key;
    struct vnode *node;

    if (child == NULL || parent == current_proc->root) {
        return depth;
    }

    ret = 0;

    dict_get_keys(&child->children, &iter);

    while (iter_move_next(&iter, (void**)&key)) {
        dict_get(&child->children, key, (void**)&node);  

        if (node != parent) {
            continue;
        }

        components[depth] = key;
        
        ret = depth + 1;

        if (node->parent->parent) {
            ret = vn_resolve_name_r(node->parent->parent, node->parent, components, ret);
        }
        
        break;
    }

    iter_close(&iter);
    
    return ret;
}

void
vn_resolve_name(struct vnode *vn, char *buf, int bufsize)
{
    int i;
    int ncomponents;
    size_t component_size;
    char *components[256];

    ncomponents = vn_resolve_name_r(vn->parent, vn, components, 0);

    for (i = ncomponents - 1; i >= 0; i--) {
        *(buf++) = '/';
        component_size = strlen(components[i]);
        strcpy(buf, components[i]);
        buf += component_size;
    }
   
    if (ncomponents == 0) {
        *(buf++) = '/';
    }

    *(buf++) = 0;
}


int     
vop_fchmod(struct vnode *node, mode_t mode)
{       
    struct vops *ops;
    
    ops = node->ops;
    
    if (ops && ops->chmod) {
        return ops->chmod(node, mode);
    }

    return -(ENOTSUP);
}

int
vop_fchown(struct vnode *node, uid_t owner, gid_t group)
{
    struct vops *ops;
    
    ops = node->ops;

    if (ops && ops->chown) {
        return ops->chown(node, owner, group);
    }

    return -(ENOTSUP);
}

int
vop_ftruncate(struct vnode *node, off_t length)
{
    struct vops *ops;
    
    ops = node->ops;

    if (ops && ops->truncate) {
        return ops->truncate(node, length);
    }

    return -(ENOTSUP);
}

int
vop_read(struct vnode *node, char *buf, size_t nbyte, off_t offset)
{
    struct vops *ops;
    
    ops = node->ops;

    if (ops && ops->read) {
        return ops->read(node, buf, nbyte, offset);
    }

    return -(EPERM);
}

int
vop_readdirent(struct vnode *node, struct dirent *dirent, int dirno)
{
    struct vops *ops;
    
    ops = node->ops;

    if (ops && ops->readdirent) {
        return ops->readdirent(node, dirent, dirno);
    }

    return -(EPERM);
}

int
vop_stat(struct vnode *node, struct stat *stat)
{
    struct vops *ops;
    
    ops = node->ops;

    if (ops->stat) {
        return ops->stat(node, stat);
    }

    return -(ENOTSUP);
}

int
vop_write(struct vnode *node, const char *buf, size_t nbyte, off_t offset)
{
    int written;
    struct vops *ops;
    
    ops = node->ops;

    if (ops->write) {
        written = ops->write(node, buf, nbyte, offset);

        return written;
    }

    return -(EPERM);
}