/*
 * proc_sysctl.c
 *
 * Process related sysctls
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
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/world.h>

static void
fill_proc_info(struct kinfo_proc *proc_info, struct proc *proc)
{
    char *tty;

    proc_info->pid = proc->pid;
    proc_info->uid = proc->creds.uid;
    proc_info->gid = proc->creds.gid;
    proc_info->sid = PROC_SID(proc);
    proc_info->pgid = PROC_PGID(proc);
    
    if (proc->parent) {
        proc_info->ppid = proc->parent->pid;
    }
    
    proc_info->stime = proc->start_time + time_delta.tv_sec;

    tty = proc_getctty(proc);

    strncpy(proc_info->cmd, proc->name, 255);

    if (tty) {
        strncpy(proc_info->tty, tty, 31);
    } else {
        proc_info->tty[0] = '\x00';
    }
}

static int
sysctl_proc_all(void *buf, size_t *len)
{
    extern struct list process_list;

    int i;
    int maxprocs;
    list_iter_t iter;
    struct kinfo_proc *procs;
    struct proc *proc;

    if (!buf && len) {
        *len = (LIST_SIZE(&process_list)+16)*sizeof(struct kinfo_proc);
        return 0;
    }

    if (!len) {
        return -1;
    }

    maxprocs = *len / sizeof(struct kinfo_proc);

    list_get_iter(&process_list, &iter);

    procs = buf;    
    i = 0;

    while (iter_move_next(&iter, (void**)&proc) && i < maxprocs) {
        if (WORLD_CAN_SEE_PROC(current_proc->world, proc)) {
            fill_proc_info(&procs[i++], proc);
        }
    } 

    *len = (i*sizeof(struct kinfo_proc));

    iter_close(&iter);

    return 0;
}

int
proc_sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
    switch (name[0]) {
        case KERN_PROC_ALL:
            return sysctl_proc_all(oldp, oldlenp);
    }
    return -1;
}

