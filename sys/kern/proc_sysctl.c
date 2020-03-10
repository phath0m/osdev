#include <ds/list.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/sysctl.h>
#include <sys/types.h>

static void
fill_proc_info(struct kinfo_proc *proc_info, struct proc *proc)
{
    proc_info->pid = proc->pid;
    proc_info->uid = proc->creds.uid;
    proc_info->gid = proc->creds.gid;
    proc_info->sid = PROC_SID(proc);
    proc_info->pgid = PROC_PGID(proc);
    
    if (proc->parent) {
        proc_info->ppid = proc->parent->pid;
    }
    
    proc_info->stime = proc->start_time;

    char *tty = proc_getctty(proc);

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

    if (!buf && len) {
        *len = (LIST_SIZE(&process_list)+16)*sizeof(struct kinfo_proc);
        return 0;
    }

    if (!len) {
        return -1;
    }

    int maxprocs = *len / sizeof(struct kinfo_proc);

    list_iter_t iter;
    list_get_iter(&process_list, &iter);

    struct kinfo_proc *procs = buf;    
    struct proc *proc;
    int i = 0;

    while (iter_move_next(&iter, (void**)&proc) && i < maxprocs) {
        fill_proc_info(&procs[i++], proc);
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

