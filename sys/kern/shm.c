#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/wait.h>
// delete me
#include <sys/systm.h>

struct shm_object {
    size_t              size;
    void *              addr;
    struct vm_space *   space;
};


static intptr_t shm_mmap(struct file *fp, uintptr_t addr, size_t size, int prot, off_t offset);
static int shm_truncate(struct file *fp, off_t length);

struct fops shm_ops = {
    .close = NULL,
    .destroy = NULL,
    .mmap = shm_mmap,
    .truncate = shm_truncate
};

static struct dict shm_objects;

int
shm_open(struct proc *proc, struct file **result, const char *name, int oflag, mode_t mode)
{    
    struct vnode *node;
    struct cred *creds = &proc->creds;

    if (!dict_get(&shm_objects, name, (void**)&node)) {
        node = vn_new(NULL, NULL, NULL);

        struct shm_object *shm = calloc(1, sizeof(struct shm_object));
        node->state = shm;
        node->uid = creds->uid;
        node->gid = creds->gid;
        node->mode = mode;

        dict_set(&shm_objects, name, node);
    }

    bool can_write = (node->uid == creds->uid && (node->mode & S_IWUSR)) ||
                     (node->gid == creds->gid && (node->mode & S_IWGRP)) ||
                     (node->mode & S_IWOTH);

    bool can_read = (node->uid == creds->uid && (node->mode & S_IRUSR)) ||
                    (node->gid == creds->gid && (node->mode & S_IRGRP)) ||
                    (node->mode & S_IROTH);

    bool will_write = (oflag & O_WRONLY);

    if (will_write && !can_write) {
        return -(EPERM);
    }

    if (!can_read) {
        return -(EPERM);
    }

    struct file *fp = file_new(&shm_ops, node);
    fp->state = node->state;
    *result = fp;

    return 0;  
}

int
shm_unlink(struct proc *proc, const char *name)
{
    struct vnode *node;
    struct shm_object *object;

    if (dict_get(&shm_objects, name, (void**)&node)) {
        object = (struct shm_object*)node->state;
    } else {
        return -(ENOENT);
    }

    struct cred *creds = &proc->creds;

    bool can_delete = (node->uid == creds->uid && (node->mode & S_IWUSR)) ||
                      (node->gid == creds->gid && (node->mode & S_IWGRP)) ||
                      (node->mode & S_IWOTH);

    if (!can_delete) {
        return -(EPERM);
    }

    dict_remove(&shm_objects, name);
    free(object);

    return 0;
}

static intptr_t
shm_mmap(struct file *fp, uintptr_t addr, size_t size, int prot, off_t offset)
{
    struct shm_object *obj = (struct shm_object*)fp->state;

    if (size != obj->size) {
        return -1;
    }

    struct vm_space *space = current_proc->thread->address_space;

    if (!obj->space) {
        /* its not mapped already, we've got to allocate and map space */
        obj->space = space; 
        obj->addr = vm_map(space, NULL, size, prot);
        return (intptr_t)obj->addr;
    }

    /* in this case, its already mapped; just gotta share it with the calling process */
    return (intptr_t)vm_share(space, obj->space, NULL, obj->addr, obj->size, prot);
}

static int
shm_truncate(struct file *fp, off_t length)
{
    struct shm_object *obj = (struct shm_object*)fp->state;

    obj->size = length;

    return 0;
}
