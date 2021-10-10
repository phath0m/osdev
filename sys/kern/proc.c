/*
 * proc.c - process structure implementations
 *
 * Manages objects in kernel spaces related to processes
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
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/world.h>

static int next_pid;

int proc_count = 0;
/*
 * List of all processes running
 */
struct list process_list;
struct list thread_list;
struct list pgrp_list;

/* pool to manage allocation of proc structures */
struct pool proc_pool;
struct pool thread_pool;

struct pgrp *
pgrp_find(pid_t pgid)
{
    list_iter_t iter;

    struct pgrp *pgrp;
    struct pgrp *ret;

    list_get_iter(&pgrp_list, &iter);

    ret = NULL;
    
    while (iter_move_next(&iter, (void**)&pgrp)) {
        if (pgrp && pgrp->pgid == pgid) {
            ret = pgrp;
            break;
        }
    }

    iter_close(&iter);

    return ret;
}

void
pgrp_leave_session(struct pgrp *group, struct session *session)
{
    list_remove(&session->groups, group);

    if (LIST_SIZE(&session->groups) == 0) {
        free(session);
        list_remove(&pgrp_list, group);
    }
}

struct pgrp *
pgrp_new(struct proc *leader, struct session *session)
{
    struct pgrp *group;

    group = calloc(1, sizeof(struct pgrp));

    group->leader = leader;
    group->pgid = leader->pid;
    group->session = session;

    list_append(&group->members, leader);
    list_append(&session->groups, group);
    list_append(&pgrp_list, group);

    return group;
}

void
proc_destroy(struct proc *proc)
{
    int i;
    list_iter_t iter;
    struct file *fp;
    struct proc *orphan;
    struct thread *thread;

    KASSERT(proc != LIST_FIRST(&process_list), "init died");
    KASSERT(proc->parent != NULL, "cannot have NULL parent");

    proc_leave_group(proc, proc->group);
    proc_leave_world(proc, proc->world);

    thread = proc->thread;

    for (i = 0; i < 4096; i++) {
        fp = proc->files[i];

        if (fp) {
            fop_close(fp);
        }
    }

    vm_space_destroy(thread->address_space);

    list_remove(&process_list, proc);

    list_get_iter(&proc->children, &iter);

    while (iter_move_next(&iter, (void**)&orphan)) {
        orphan->parent = LIST_FIRST(&process_list);
    }

    iter_close(&iter);

    list_remove(&proc->parent->children, proc);

    list_destroy(&proc->children, false);
    list_destroy(&proc->threads, false);

    wq_empty(&proc->waiters);

    proc_count--;
    
    /* send SIGCHLD */ 
    proc_kill(proc->parent, 17);

    pool_put(&proc_pool, proc);
}

struct proc *
proc_find(int pid)
{
    list_iter_t iter;

    struct proc *proc;
    struct proc *ret;

    list_get_iter(&process_list, &iter);

    while (iter_move_next(&iter, (void**)&proc)) {
        if (proc && proc->pid == pid) {
            ret = proc;
            break;
        }
    }

    iter_close(&iter);

    return ret;
}

void
proc_getcwd(struct proc *proc, char *buf, int bufsize)
{
    return vn_resolve_name(proc->cwd, buf, bufsize);
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

void
proc_leave_world(struct proc *proc, struct world *world)
{
    list_remove(&world->members, proc);

    if (LIST_SIZE(&world->members) == 0) {
        free(world);
    }   
}

char *
proc_getctty(struct proc *proc)
{
    struct session *session;

    session = PROC_GET_SESSION(proc);
    
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
    struct proc *proc;

    proc = pool_get(&proc_pool);

    proc->start_time = time_second;
    proc->pid = proc_get_new_pid();

    list_append(&process_list, proc);

    proc_count++;

    return proc;
}

struct session *
session_new(struct proc *leader)
{
    struct session *session;

    session = calloc(1, sizeof(struct session));

    session->sid = leader->pid;
    session->leader = leader;

    return session;  
}

void
thread_destroy(struct thread *thread)
{
    struct proc *proc;

    proc = thread->proc;

    list_destroy(&thread->joined_queues, true);

    free((void*)thread->stack_base);

    pool_put(&thread_pool, thread);

    if (!proc) {
        return;
    }

    list_remove(&proc->threads, thread);

    if (LIST_SIZE(&proc->threads) == 0) {
        proc_destroy(proc);
    }
}

struct thread *
thread_new(struct vm_space *space)
{
    struct thread *thread;

    thread = pool_get(&thread_pool);

    if (space) {
        thread->address_space = space;
    } else {
        thread->address_space = vm_space_new();
    }

    return thread;
}



