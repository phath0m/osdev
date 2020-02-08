#ifndef _SYS_FILE_H
#define _SYS_FILE_H

#include <sys/types.h>

struct file {
    struct vnode *      node;
    int                 flags;
    int                 refs;
    uint64_t            position;
};

struct file *file_new(struct vnode *node);
struct file *vfs_duplicate_file(struct file *file);

#endif
