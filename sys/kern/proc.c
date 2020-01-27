#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/vm.h>

static int next_pid;

int proc_count = 0;
/*
 * List of all processes running
 */
struct list process_list;

void
pgrp_leave_session(struct pgrp *group, struct session *session)
{
    list_remove(&session->groups, group);

    if (LIST_SIZE(&session->groups) == 0) {
        free(session);
    }
}

struct pgrp *
pgrp_new(struct proc *leader, struct session *session)
{
    struct pgrp *group = calloc(1, sizeof(struct pgrp));

    group->leader = leader;
    group->pgid = leader->pid;
    group->session = session;

    list_append(&session->groups, group);

    return group;
}

void
proc_destroy(struct proc *proc)
{
    KASSERT(proc != LIST_FIRST(&process_list), "init died");
    KASSERT(proc->parent != NULL, "cannot have NULL parent");

    proc_leave_group(proc, proc->group);

    struct thread *thread = proc->thread;

    for (int i = 0; i < 4096; i++) {
        struct file *fp = proc->files[i];

        if (fp) {
            fops_close(fp);
        }
    }

    vm_space_destroy(thread->address_space);

    list_remove(&process_list, proc);

    list_iter_t iter;
    list_get_iter(&proc->children, &iter);

    struct proc *orphan;

    while (iter_move_next(&iter, (void**)&orphan)) {
        orphan->parent = LIST_FIRST(&process_list);
    }

    iter_close(&iter);

    list_remove(&proc->parent->children, proc);

    list_destroy(&proc->children, false);
    list_destroy(&proc->threads, false);

    wq_empty(&proc->waiters);

    proc_count--;

    free(proc);
}

static int
getcwd_r(struct vfs_node *child, struct vfs_node *parent, char **components, int depth)
{
    if (child == NULL) {
        return depth;
    }

    int ret = 0;

    list_iter_t iter;

    dict_get_keys(&child->children, &iter);

    char *key;
    
    while (iter_move_next(&iter, (void**)&key)) {
        struct vfs_node *node;
 
        dict_get(&child->children, key, (void**)&node);  

        if (node != parent) {
            continue;
        }

        components[depth] = key;
        
        ret = depth + 1;

        if (node->parent->parent) {
            ret = getcwd_r(node->parent->parent, node->parent, components, ret);
        }
        
        break;
    }

    iter_close(&iter);
    
    return ret;
}

void
proc_getcwd(struct proc *proc, char *buf, int bufsize)
{
    char *components[256];

    int ncomponents = getcwd_r(proc->cwd->parent, proc->cwd, components, 0);

    for (int i = ncomponents - 1; i >= 0; i--) {
        *(buf++) = '/';
        size_t component_size = strlen(components[i]);
        strcpy(buf, components[i]);
        buf += component_size;
        printf("Subcomponent: %s\n", components[i]);
    }
    
    *(buf++) = 0;
}

void
proc_leave_group(struct proc *proc, struct pgrp *group)
{
    list_remove(&group->members, proc);

    if (LIST_SIZE(&group->members) == 0) {
        pgrp_leave_session(group, group->session);
        free(group);
    }   
}

int
proc_dup(int oldfd)
{
    int newfd = proc_getfildes();

    return proc_dup2(oldfd, newfd);
}

int
proc_dup2(int oldfd, int newfd)
{
    if (oldfd >= 4096 || newfd >= 4096) {
        return -1;
    }

    if (oldfd < 0 || newfd < 0) {
        return -1;
    }

    struct file *existing_fp = current_proc->files[newfd];

    if (existing_fp) {
        fops_close(existing_fp);   
    }

    struct file *fp = current_proc->files[oldfd];
   
    if (!fp) {
        return -1;
    }

    current_proc->files[newfd] = fp;

    INC_FILE_REF(fp);
    
    return newfd;
}

struct file *
proc_getfile(int fildes)
{   
    if (fildes >= 4096) {
        return NULL;
    }
    return current_proc->files[fildes];
}

int
proc_getfildes()
{   
    for (int i = 0; i < 4096; i++) { 
        if (!current_proc->files[i]) {
            return i;
        }
    }
    
    return -EMFILE;
}

char *
proc_getctty(struct proc *proc)
{
    if (proc->files[0]) {
        struct file *file = proc->files[0];

        if (!file || !file->node || !file->node->device) {
            return NULL;
        }

        if (device_isatty(file->node->device)) {
            return file->node->device->name;
        }
    }

    return NULL;
}

struct proc *
proc_new()
{
    struct proc *proc = (struct proc*)calloc(0, sizeof(struct proc));

    proc->start_time = time(NULL);    
    proc->pid = ++next_pid;

    list_append(&process_list, proc);

    proc_count++;

    return proc;
}

int
proc_newfildes(struct file *file)
{
    for (int i = 0; i < 4096; i++) {
        if (!current_proc->files[i]) {
            current_proc->files[i] = file;
            return i;
        }
    }

    return -EMFILE;
}

struct session *
session_new(struct proc *leader)
{
    struct session *session = calloc(1, sizeof(struct session));

    session->leader = leader;

    return session;  
}
