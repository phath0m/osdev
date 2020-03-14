/*
 * misc_syscalls.c - miscellaneous systemcalls
 *
 * Basically system call handlers that didn't warrant having their own file
 * created.
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
#include <sys/errno.h>
#include <sys/string.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/sysctl.h>

static int
sys_time(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(time_t *, tloc, 0, argv);

    TRACE_SYSCALL("time", "0x%p", tloc);

    return time(tloc);
}

static int
sys_uname(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(struct utsname *, buf, 0, argv);

    TRACE_SYSCALL("uname", "0x%p", buf);

    strncpy(buf->sysname, ELYSIUM_SYSNAME, sizeof(buf->sysname));
    strncpy(buf->release, ELYSIUM_RELEASE, sizeof(buf->release));
    strncpy(buf->version, ELYSIUM_VERSION, sizeof(buf->version));
    strncpy(buf->machine, ELYSIUM_TARGET, sizeof(buf->machine));
    
    return 0;
}


static int
sys_sysctl(struct thread *th, syscall_args_t argv)
{
    DEFINE_SYSCALL_PARAM(struct sysctl_args *, args, 0, argv);

    // TODO: validate pointers
    
    return sysctl(args->name, args->namelen, args->oldp, args->oldlenp, args->newp, args->newlen);
}

void
misc_syscalls_init()
{
    register_syscall(SYS_TIME, 1, sys_time);
    register_syscall(SYS_UNAME, 1, sys_uname);
    register_syscall(SYS_SYSCTL, 1, sys_sysctl);
}
