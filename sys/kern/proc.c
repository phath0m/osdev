#include <ds/list.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/vm.h>

static int next_pid;

int proc_count = 0;
/*
 * List of all processes running
 */
struct list process_list;
struct list thread_list;

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

    list_append(&group->members, leader);
    list_append(&session->groups, group);

    return group;
}

void
proc_destroy(struct proc *proc)
{
    KASSERT(proc != LIST_FIRST(&process_list), "init died");
    KASSERT(proc->parent != NULL, "cannot have NULL parent");

    printf("leave group (proc=0x%p group=0x%p)\n", proc, proc->group);

    proc_leave_group(proc, proc->group);

    struct thread *thread = proc->thread;

    for (int i = 0; i < 4096; i++) {
        struct file *fp = proc->files[i];

        if (fp) {
            vops_close(fp);
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

struct proc *
proc_getbypid(int pid)
{
    list_iter_t iter;

    list_get_iter(&process_list, &iter);

    struct proc *proc;
    struct proc *ret;
    while (iter_move_next(&iter, (void**)&proc)) {
        if (proc && proc->pid == pid) {
            ret = proc;
            break;
        }
    }

    iter_close(&iter);

    return ret;
}
static int
getcwd_r(struct vnode *child, struct vnode *parent, char **components, int depth)
{
    if (child == NULL) {
        return depth;
    }

    int ret = 0;

    list_iter_t iter;

    dict_get_keys(&child->children, &iter);

    char *key;
    
    while (iter_move_next(&iter, (void**)&key)) {
        struct vnode *node;
 
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
    }
   
    if (ncomponents == 0) {
        *(buf++) = '/';
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

char *
proc_getctty(struct proc *proc)
{
    struct session *session = PROC_GET_SESSION(proc);
    
    if (session->ctty) {
        return session->ctty->name;
    }

    return NULL;
}

pid_t
proc_get_new_pid()
{
    return ++next_pid;
}

struct proc *
proc_new()
{
    struct proc *proc = (struct proc*)calloc(0, sizeof(struct proc));

    proc->start_time = time(NULL);    
    proc->pid = proc_get_new_pid();
    list_append(&process_list, proc);

    proc_count++;

    return proc;
}

struct session *
session_new(struct proc *leader)
{
    struct session *session = calloc(1, sizeof(struct session));

    session->leader = leader;

    return session;  
}

struct thread *
thread_new()
{
    struct thread *thread = calloc(1, sizeof(struct thread));

    return thread;
}
