/*
 * sysctl.h
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
#ifndef _SYS_SYSCTL_H
#define _SYS_SYSCTL_H
#ifdef __cplusplus
extern "C" {
#endif

//#include <sys/limits.h>
#define PATH_MAX 256
#include <sys/stat.h>
#include <sys/types.h>

#define CTL_KERN            0x01

#define KERN_PROC           1
#define KERN_PROC_ALL       1
#define KERN_PROC_VMMAP     2
#define KERN_PROC_FILES     3

struct kinfo_proc {
    pid_t   pid;
    pid_t   ppid;
    uid_t   uid;
    uid_t   sid;
    uid_t   pgid;
    gid_t   gid;
    char    cmd[256];
    char    tty[32];
    time_t  stime;
};

struct kinfo_ofile {
    int     fd;
    int     type;
    int     seek;
    dev_t   dev;
    char    path[PATH_MAX+1];
};

struct kinfo_vmentry {
    uintptr_t   start;
    uintptr_t   end;
    int         prot;
};

#ifdef __KERNEL__
int kern_sysctl(int *, int, void *, size_t *, void *, size_t);
#endif

int sysctl(int *, int, void *, size_t *, void *, size_t);

#ifdef __cplusplus
}
#endif
#endif /* _SYS_SYSCTL_H */
