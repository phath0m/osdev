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
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vm.h>
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

static void
fill_vmentry_info(struct kinfo_vmentry *vm_entry, uintptr_t start, uintptr_t stop, int prot)
{
    vm_entry->start = start;
    vm_entry->end = stop;
    vm_entry->prot = prot;
}

static void
fill_file_info(struct kinfo_ofile *ofile, int fd, struct file *filep)
{
    struct vnode *vn;
    struct stat sb;

    ofile->fd = fd;

    FOP_STAT(filep, &sb);

    ofile->type = sb.st_mode;
    ofile->dev = sb.st_dev;

    if (FOP_GETVN(filep, &vn) == 0) {
        vn_resolve_name(vn, ofile->path, sizeof(ofile->path));        
    }
}

static int
sysctl_proc_all(void *buf, size_t *lenp)
{
    extern struct list process_list;

    int i;
    int maxprocs;
    list_iter_t iter;
    struct kinfo_proc *procs;
    struct proc *proc;

    if (!buf && lenp) {
        *lenp = (LIST_SIZE(&process_list)+16)*sizeof(struct kinfo_proc);
        return 0;
    }

    if (!lenp) {
        return -1;
    }

    maxprocs = *lenp / sizeof(struct kinfo_proc);

    list_get_iter(&process_list, &iter);

    procs = buf;    
    i = 0;

    while (iter_move_next(&iter, (void**)&proc) && i < maxprocs) {
        if (WORLD_CAN_SEE_PROC(current_proc->world, proc)) {
            fill_proc_info(&procs[i++], proc);
        }
    } 

    *lenp = (i*sizeof(struct kinfo_proc));

    iter_close(&iter);

    return 0;
}

static int
sysctl_proc_vmmap(int *name, int namelen, void *buf, size_t *lenp)
{
    int pid;
    int i;
    int maxentries;
    list_iter_t iter;
    uintptr_t entry_start;
    uintptr_t entry_stop;

    struct kinfo_vmentry *entries;
    struct proc *proc;
    struct vm_block *block;

    if (namelen != 1) {
        return -(EINVAL);
    }

    pid = name[0];
    proc = proc_find(pid);

    if (proc == NULL || !WORLD_CAN_SEE_PROC(current_proc->world, proc)) {
        return -(ESRCH);
    }

    if (!buf && lenp) {
        *lenp = (LIST_SIZE(&proc->thread->address_space->map)+16)*sizeof(struct kinfo_vmentry);
        return 0;
    }

    if (!lenp) {
        return -(EINVAL);
    }

    list_get_iter(&proc->thread->address_space->map, &iter);

    maxentries = *lenp / sizeof(struct kinfo_vmentry);
    i = 0;
    entry_start = 0;
    entry_stop = 0;
    entries = buf;

    while (iter_move_next(&iter, (void**)&block) && i < maxentries) {
        if (VM_IS_KERN(block->prot)) continue;

        if (entry_start == 0) {
            entry_start = block->start_virtual;
            entry_stop = entry_start + block->size;
        } else if (block->start_virtual != entry_stop) {
            fill_vmentry_info(&entries[i++], entry_start, entry_stop, block->prot);
            entry_start = 0;
            entry_stop = 0;
        } else {
            entry_stop += block->size;
        }
    }

    if (entry_start != 0) {
        fill_vmentry_info(&entries[i++], entry_start, entry_stop, block->prot);
    }

    *lenp = (i*sizeof(struct kinfo_vmentry));

    iter_close(&iter);

    return 0;
}

static int
sysctl_proc_files(int *name, int namelen, void *buf, size_t *lenp)
{
    int pid;
    int i;
    int r;
    int maxentries;

    struct kinfo_ofile *entries;
    struct proc *proc;

    if (namelen != 1) {
        return -(EINVAL);
    }

    pid = name[0];
    proc = proc_find(pid);

    if (proc == NULL || !WORLD_CAN_SEE_PROC(current_proc->world, proc)) {
        return -(ESRCH);
    }

    if (!buf && lenp) {
        *lenp = (PROC_COUNT_FILEDESC(proc)+16)*sizeof(struct kinfo_ofile);
        return 0;
    }

    if (!lenp) {
        return -(EINVAL);
    }

    maxentries = *lenp / sizeof(struct kinfo_ofile);
    entries = buf;

    r = 0;

    for (i = 0; i < maxentries; i++) {
        if (proc->files[i]) {
            fill_file_info(&entries[r++], i, proc->files[i]);
        }
    }

    *lenp = sizeof(struct kinfo_ofile)*r;

    return 0;
}



int
proc_sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
    switch (name[0]) {
        case KERN_PROC_ALL:
            return sysctl_proc_all(oldp, oldlenp);
        case KERN_PROC_VMMAP:
            return sysctl_proc_vmmap(&name[1], namelen - 1, oldp, oldlenp);
        case KERN_PROC_FILES:
            return sysctl_proc_files(&name[1], namelen - 1, oldp, oldlenp);
        default:
            break;
    }
    return -1;
}

